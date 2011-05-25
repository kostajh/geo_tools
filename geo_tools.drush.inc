<?php

/**
 * Implementation of hook_drush_help().
 */
function geo_tools_drush_help($section) {
  switch ($section) {
    case 'drush:geo-tools-list':
      $output = dt("Lists data from CSV of pledge signers.");
      return $output;
    case 'drush:geo-tools-export':
      $output = dt("Exports data from the pledge CSV to JSON data");
      return $output;
  }
}

/**
 * Implementation of hook_drush_command().
 */
function geo_tools_drush_command() {
  $items['geo-tools-list'] = array(
    'description' => 'Lists data from CSV of pledge signers.',
  );

  $items['geo-tools-export'] = array(
    'description' => 'Convert pledge data to JSON.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );
  
  $items['geo-tools-php'] = array(
    'description' => 'Pass some values to Yahoo geoservice, get some back',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  
  $items['geo-tools-tally'] = array(
    'description' => 'Takes a CSV and tallies results',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  
  return $items;
}

function drush_geo_tools_list($csv = NULL) {
  $path = drupal_get_path('module', 'geo_tools');
  $csv = $path . '/' . $csv;
  drush_print($csv);
  if (empty($csv)) {
    drush_die('CSV file not found');
    return;
  }
  
  $write_csv = $csv . '-mod.csv';

  $row = 1;
  $table = array();
  $handle = fopen($csv, 'r');
  $handle_write = fopen($write_csv, 'w');
      while (($data = fgetcsv($handle, 1000, ",")) !== FALSE) {
        $location = array();
        $num = count($data);

        $row++;
        $row_data = array();
         for ($c = 0; $c < $num; $c++) {
           $row_data[$c] = $data[$c];
           if ($data[$c]) {
             if ($c == 10) {
               $location['street'] = $data[$c];
             }
             if ($c == 12) {
               $location['city'] = $data[$c];
             }
             if ($c == 13) {
               $location['state'] = $data[$c];
             }
             if ($c == 14) {
               $location['zip'] = $data[$c];
             }
             if ($c == 23) {
               $location['country'] = $data[$c];
             }
           }

        } // end loop through row
        if ($row == 1) {
          $row++;
          // Print headers
          $ret = fputcsv($handle_write, $row_data, ',');
          continue;
        }
        if ($location) {
          $loc_data = implode(', ', $location);
          drush_print($loc_data);
          // Run the geocoding, then write to fields 25 and 26
          if ($geodata = geo_tools_get_location($loc_data)) {
            $row_data[25] = $geodata['latitude'];
            $row_data[26] = $geodata['longitude'];              
          }
          $ret = fputcsv($handle_write, $row_data, ',');
        }        
      } // end while loop through entire CSV
      fclose($handle);
      fclose($handle_write);
  
}

/**
 * Pass in a location string, get back latitude and longitude
 */
function geo_tools_get_location($loc_data = array()) {
  if (empty($loc_data)) {
    return FALSE;
  }
  $app_id = 'S2xkoZ7e';
  $query = array(
    'app_id' => $app_id, 
    'flags' => 'P', 
    'location' => $loc_data,
  );

  $url = url('http://where.yahooapis.com/geocode', array(
    'query' => $query, 
    'external' => TRUE,
  ));

  $http_reply = drupal_http_request($url);
  if ($http_reply->data) {
    $response = unserialize($http_reply->data);
    if ($response['ResultSet']['Error'] == 0) {
      return $response['ResultSet']['Result'][0];
    }
  } else {
    return FALSE;
  }
}

function drush_geo_tools_php($args) {
  
  $app_id = 'S2xkoZ7e';
  
  $query = array(
    'app_id' => $app_id, 
    'flags' => 'P', 
    'location' => 'Chapel Hill NC',
  );

  $url = url('http://where.yahooapis.com/geocode', array(
    'query' => $query, 
    'external' => TRUE,
  ));

  $http_reply = drupal_http_request($url);
  drush_print(print_r($http_reply));
  if ($http_reply->data) {
    $response = unserialize($http_reply->data);
    drush_print(print_r($response));
  }
}

function drush_geo_tools_tally() {
  $import_file = drush_prompt('Name of CSV to work with: ', 'pledge-data.csv', TRUE);
  $export_file = drush_prompt('Name of CSV to export to', 'geo_tools_export.csv', TRUE);
  $path = drupal_get_path('module', 'geo_tools');
  $csv = $path . '/' . $import_file;
  if (empty($csv)) {
    drush_die('CSV file not found');
    return;
  }
  $row = 1;
  $location_data = array();
  $table = array();
  $handle = fopen($csv, 'r');
  // Start looping through CSV
  while (($data = fgetcsv($handle, 1000, ",")) !== FALSE) {
    $tally = FALSE;
    $num = count($data);
    $row++;
    $row_data = array();
    for ($c = 0; $c < $num; $c++) {
       $row_data[$c] = $data[$c];
       if ($c == 27) {
         $row_data[$c] = 1;
       }
    } // end loop through row

    // We now have a row of data including the city and state
    // Loop through the location_data array that we are building up and see if a match exists
    // If so, increment the tally for the match.
    foreach ($location_data as &$loc) {
      // If city and state match, then increment the value in the array
      
      if (strtolower($loc[12]) == strtolower($row_data[12]) && strtolower($loc[13]) == strtolower($row_data[13]) && !empty($loc[12])) {
        drush_print("Found a match for $loc[12], $loc[13] for $loc[7] $loc[8]");
        $loc[27]++;              
        $tally = TRUE;
        break;
      }
    }
    
    if (!$tally) {
      drush_print("Adding new row for $loc[12], $loc[13]");
      $row_data[27] = 1;
      $location_data[] = $row_data;          
    }

  } // end while loop through entire CSV
  fclose($handle);
  // Write new CSV
  
 $handle_write = fopen($path . '/' . $export_file, 'w');
 foreach ($location_data as $row_data) {
   $ret = fputcsv($handle_write, $row_data, ',');
 } 
 fclose($handle_write);
  
  drush_print(print_r($location_data));
}