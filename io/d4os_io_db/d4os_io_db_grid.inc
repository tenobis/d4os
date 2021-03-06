<?php
/**
 * @package    d4os_io_db
 * @subpackage   grid
 * @copyright  Copyright (C) 2010 Wene - ssm2017 Binder ( S.Massiaux ). All rights reserved.
 * @license    GNU/GPL, http://www.gnu.org/licenses/gpl-2.0.html
 * D4os is free software. This version may have been modified pursuant
 * to the GNU General Public License, and as distributed it includes or
 * is derivative of works licensed under the GNU General Public License or
 * other free or open source software licenses.
 */

/**
 * Get a region object
 * @param Array An array containing filters like $array=array("name='test'");
 * @return Object|bool An object containing region fields or False if error
 */
function d4os_io_db_os_region_load($array = array()) {
  // Dynamically compose a SQL query:
  $query = array();
  $params = array();

  if (!is_array($array)) {
    return FALSE;
  }

  foreach ($array as $key => $value) {
    $query[]= "LOWER($key) = LOWER('%s')";
    $params[] = $value;
  }
  d4os_io_db_set_active('os_grid');
  $result = db_query('SELECT * FROM {regions} WHERE '. implode(' AND ', $query), $params);
  $region = db_fetch_object($result);
  d4os_io_db_set_active('default');
  return $region;
}

/**
 * Get a region object by uuid (for the monitoring tools)
 * @param Array An array of regions uuids
 * @return Array An array of objects
 */
function d4os_io_db_os_regions_load_by_uuids($uuids = array()) {
  if (!is_array($uuids) || !count($uuids)) {
    return FALSE;
  }
  $regions = array();
  $owners = array();
  // get regions infos
  d4os_io_db_set_active('os_grid');
  $result = db_query("SELECT *, CONCAT('http://', serverIP, ':', serverPort) AS Uri"
                  . " FROM {regions}"
                  . " WHERE uuid IN ('". implode("','", $uuids). "')"
                  . " ORDER BY Uri");
  while ($region = db_fetch_object($result)) {
    $regions[] = $region;
    $owners[] = $region->owner_uuid;
  }
  d4os_io_db_set_active('default');

  // get owners infos
  $owners = array_unique($owners);
  $owners_infos = d4os_io_db_os_users_load_by_uuids($owners);
  $regions_count = count($regions);
  for ($i=0; $i<$regions_count; ++$i) {
    foreach ($owners_infos as $owner) {
      if ($regions[$i]->owner_uuid == $owner->UUID) {
        $regions[$i]->owner = $owner->name;
      }
    }
  }
  return $regions;
}

/**
 * Get a list of regions for the stats
 * @return Array of objects
 */
function d4os_io_db_os_regions_get_all_regions() {
  $grid_regions = array();
  d4os_io_db_set_active('os_grid');
  $result = db_query('SELECT * FROM {regions}');
  while ($grid_region = db_fetch_object($result)) {
    $grid_regions[] = $grid_region;
  }
  d4os_io_db_set_active('default');
  return $grid_regions;
}

/**
 * Get a list of regions obects with paging
 * @param int start page
 * @param int number of entries
 * @param Array An array of drupal field headers
 * @return Array An array containing a list of regions
 */
function d4os_io_db_os_regions_list_paged($page = 0, $limit = 5, $headers) {
  if (!isset($_GET['page'])) {
    $_GET['page'] = $page;
  }
  if (!isset($_GET['order'])) {
    $_GET['order'] = 'regionName';
  }
  if (!isset($_GET['sort'])) {
    $_GET['sort'] = 'DESC';
  }
  d4os_io_db_set_active('os_grid');
  $result = pager_query('SELECT * FROM {regions} %s', $limit, 0, NULL, array(tablesort_sql($headers)));
  while ($region = db_fetch_object($result)) {
    $items[] = $region;
  }
  d4os_io_db_set_active('default');
  return $items;
}

/**
 * Get the quantity of regions
 * @return Int Quantity of regions
 */
function d4os_io_db_os_regions_regions_count() {
  d4os_io_db_set_active('os_grid');
  $count = db_result(db_query("SELECT COUNT(*) FROM {regions}"));
  d4os_io_db_set_active('default');
  return $count;
}

/**
 * Return a list of regions for a html list in the events module
 * @return Array An array of regions like $regions_list[$handle] = $region->regionName;
 */
function d4os_io_db_os_regions_get_regions_array() {
  $regions = d4os_io_db_os_regions_names();
  $regions_list = array();
  foreach ($regions as $region) {
    $handle = '_'. $region->regionHandle;
    $regions_list[$handle] = $region->regionName;
  }
  return $regions_list;
}

/**
 * Return a list of regions objects ordered by regionName
 * @return Array An array of regions objects
 */
function d4os_io_db_os_regions_names() {
  d4os_io_db_set_active('os_grid');
  $result = db_query('SELECT uuid, regionHandle, regionName FROM {regions} ORDER BY regionName');
  while ($region = db_fetch_object($result)) {
    $items[] = $region;
  }
  d4os_io_db_set_active('default');
  return $items;
}

/**
 * Return an array of stats for the home page
 * @return Array An array like array('success'=>true, 'data'=>$data);
 */
function d4os_io_db_os_grid_get_stats() {
  // who is online now
  $now_online = 0;
  $online_users = array();
  d4os_io_db_set_active('os_users');
  $users = db_query("SELECT UUID, currentRegion FROM {agents}"
                            . " WHERE agentOnline = 1"
                            . " AND logintime > (UNIX_TIMESTAMP(FROM_UNIXTIME(UNIX_TIMESTAMP(now()) - 80600)))" // TODO : modif Romain du 110214-1909
                            //. " AND logintime < (UNIX_TIMESTAMP(FROM_UNIXTIME(UNIX_TIMESTAMP(now()))))"
                            //. " AND logoutTime < (UNIX_TIMESTAMP(FROM_UNIXTIME(UNIX_TIMESTAMP(now()))))"
                            . " ORDER BY logintime DESC");
  while($online_user = db_fetch_object($users)) {
    $online_users[] = $online_user;
  }
  d4os_io_db_set_active('default');

  // get the regions
  $regions_list = array();
  d4os_io_db_set_active('os_grid');
  $regions = db_query("SELECT regionName, UUID FROM {regions}");
  while ($region = db_fetch_object($regions)) {
    $regions_list[] = $region;
  }
  d4os_io_db_set_active('default');

  // check if the user is in a valid region
  foreach ($online_users as $online_user) {
    //if (d4os_array_search_recursive($online_user->currentRegion, $regions_list)) { // TODO : modif Romain du 110214-1909
      ++$now_online;
    //}
  }

  // last month online
  d4os_io_db_set_active('os_users');
  $last_month_online = db_result(db_query("SELECT count(*) FROM {agents} WHERE logintime > UNIX_TIMESTAMP(FROM_UNIXTIME(UNIX_TIMESTAMP(now()) - 2419200))"));
  d4os_io_db_set_active('default');

  // users count
  d4os_io_db_set_active('os_users');
  $users_count = db_result(db_query("SELECT count(*) FROM users"));
  d4os_io_db_set_active('default');

  // regions count
  d4os_io_db_set_active('os_grid');
  $regions_count = db_result(db_query("SELECT count(*) FROM regions"));
  d4os_io_db_set_active('default');

  $values = new stdClass;
  $values->now_online         = $now_online;
  $values->last_month_online  = $last_month_online;
  $values->users_count        = $users_count;
  $values->regions_count      = $regions_count;
  return array(
    'success' => TRUE,
    'data'    => $values,
  );
}

/**
 * Tell if the grid is responding
 * @return Bool TRUE if yes and FALSE if not
 */
function d4os_io_db_os_grid_is_alive() {
  return array(
    'success' => d4os_io_db_mysql_is_alive('os_grid', 'os grid')
  );
}

/**
 * get a list of region owners (used to assign them a role)
 * @return Array An array like array('success'=>true, 'data'=>$data);
 */
function d4os_io_db_os_regions_get_owners() {
  $owners = array();

  d4os_io_db_set_active('os_grid');
  $result = pager_query('SELECT DISTINCT owner_uuid FROM {regions}');
  while ($owner = db_fetch_object($result)) {
    $owners[] = $owner;
  }
  d4os_io_db_set_active('default');

  if (count($owners)) {
    return array(
      'success'   => TRUE,
      'data'      => $owners,
    );
  }
  return array(
    'success'   => FALSE,
    'data'      => NULL,
  );
}
