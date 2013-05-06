<?php
/**
 * @file
 * Syndication logic here.
 */

/**
 * Start a content push session.
 *
 * @see bpi_menu()
 *
 * @param string $type
 *   Request type, simple http or ajax.
 * @param int $nid
 *   Node id, whose content is to be pushed.
 * @return Array/string
 *   Ajax command set in case of ajax request, push content page
 *   markup otherwise.
 */
function bpi_push_action($type, $nid) {
  $is_ajax = ($type == 'ajax') ? TRUE : FALSE;

  if (empty($nid)) {
    drupal_goto('admin/content');
  }

  if ($is_ajax) {
    $commands = array();

    $commands[] = ajax_command_ding_popup(
      'bpi-push',
      t('Push to BPI'),
      drupal_render(drupal_get_form('bpi_push_form', $nid))
    );

    return array('#type' => 'ajax', '#commands' => $commands);
  }
  else {
    return drupal_get_form('bpi_push_form', $nid);
  }
}

/**
 * BPI push form.
 *
 * @param array $form
 *   Form structure.
 * @param array $form_state
 *   Form state values.
 * @param int $nid
 *   The node id, waiting to be pushed.
 * @return array
 *   Form structure.
 *
 * @ingroup forms
 */
function bpi_push_form($form, $form_state, $nid) {
  $form['bpi_push_nid'] = array(
    '#type' => 'hidden',
    '#value' => $nid,
  );

  $dictionaries = bpi_get_bpi_dictionaries();

  $categories = array_values($dictionaries['category']);
  $form['bpi_push_category'] = array(
    '#type' => 'select',
    '#title' => t('Category'),
    '#options' => array_combine($categories, $categories),
    '#default_value' => ','
  );

  $audiences = array_values($dictionaries['audience']);
  $form['bpi_push_audience'] = array(
    '#type' => 'select',
    '#title' => t('Audience'),
    '#options' => array_combine($audiences, $audiences),
    '#default_value' => ','
  );

  $form['bpi_push_images'] = array(
    '#type' => 'checkbox',
    '#title' => t('Push with images'),
  );

  $form['bpi_push_ccl'] = array(
    '#type' => 'checkbox',
    '#title' => t('Written under Creative Commons License'),
  );

  $form['bpi_push_cancel'] = array(
    '#type' => 'button',
    '#value' => t('Cancel'),
    '#ajax' => array(
      'callback' => 'bpi_push_cancel_ajax_callback',
    ),
  );

  $form['bpi_push_submit'] = array(
    '#type' => 'submit',
    '#value' => t('Push'),
    '#ajax' => array(
      'callback' => 'bpi_push_action_ajax_callback',
      'wrapper' => 'bpi-push-form',
    ),
  );

  return $form;
}

/**
 * Submit handler for the push node form.
 *
 * @see bpi_push_form()
 *
 * @param array $form
 *   Form structure.
 * @param array $form_state
 *   Form state values.
 *
 * @ingroup forms
 */
function bpi_push_form_submit($form, &$form_state) {
  $nid = $form_state['input']['bpi_push_nid'];
  $category = $form_state['input']['bpi_push_category'];
  $audience = $form_state['input']['bpi_push_audience'];

  if (empty($nid)) {
    return;
  }

  $node = node_load($nid);
  $bpi_content = bpi_convert_to_bpi($node, $category, $audience);

  $endpoint = variable_get('bpi_service_url', '');
  $agency_id = variable_get('bpi_agency_id', '');
  $api_key = variable_get('bpi_api_key', '');
  $secret_key = variable_get('bpi_secret_key', '');

  try {
    $bpi = new Bpi($endpoint, $agency_id, $api_key, $secret_key);
    $push_result = $bpi->push($bpi_content)->getProperties();

    if (!empty($push_result['id'])) {
      db_insert('bpi_syndicated')
        ->fields(array(
          'nid' => $nid,
          'bid' => $push_result['id'],
          'status' => BPI_PUSHED,
          'timestamp' => time(),
        ))
        ->execute();

      drupal_set_message(t('Node %title was successuflly pushed to BPI well.', array('%title' => $node->title)));
    }
  }
  catch (Exception $e) {
    watchdog(
      'bpi',
      t('Failed to push node %title, with message %message.'),
      array(
        '%title' => $node->title,
        '%message' => $e->getMessage()
      ),
      WATCHDOG_ERROR
    );
    drupal_set_message(t('Error occured when pushing %title to BPI well. Check reports for more information.', array('%title' => $node->title), 'error'));
  }
}

/**
 * AJAX callback for the cancel button in bpi push node form.
 *
 * @see bpi_push_form()
 *
 * @param array $form
 *   Form structure.
 * @param array $form_state
 *   Form state values.
 * @return array
 *   A set of ajax commands.
 *
 * @ingroup forms
 */
function bpi_push_cancel_ajax_callback($form, &$form_state) {
  $response = array(
    '#type' => 'ajax',
    '#commands' => array()
  );

  $response['#commands'][] = ajax_command_ding_popup_close('bpi-push');

  return $response;
}

/**
 * AJAX callback for the push button in bpi push node form.
 *
 * @see bpi_push_form()
 *
 * @param array $form
 *   Form structure.
 * @param array $form_state
 *   Form state values.
 * @return array
 *   A set of ajax commands.
 *
 * @ingroup forms
 */
function bpi_push_action_ajax_callback($form, &$form_state) {
  $response = array(
    '#type' => 'ajax',
    '#commands' => array()
  );

  $html = theme('status_messages');

  $response['#commands'][] = ajax_command_ding_popup(
    'bpi-push',
    t('Push to BPI'),
    $html
  );

  return $response;
}

/**
 * Convert node object to bpi-related array structure,
 * suitable for pushing to the well.
 *
 * @param stdClass $node
 *   Node object being processed.
 * @param string $category
 *   Selected category at the BPI side.
 * @param string $audience
 *   Selected audience at the BPI side.
 * @return array
 *   An array of node values, used by the BPI ws.
 */
function bpi_convert_to_bpi($node, $category, $audience) {
  $bpi_content = array();

  if ($node instanceof stdClass) {
    $bpi_content['agency_id'] = variable_get('bpi_agency_id', '');
    $bpi_content['local_id'] = $node->nid;
    $user = user_load($node->uid);
    $bpi_content['firstname'] = $user->name;
    $bpi_content['lastname'] = '';
    $bpi_content['title'] = $node->title;

    $teaser_map = variable_get('bpi_field_teaser', '');
    $body_map = variable_get('bpi_field_body', '');
    $teaser_field = field_view_field('node', $node, $teaser_map);
    $body_field = field_view_field('node', $node, $body_map);
    $teaser = '';
    $body = '';

    // Whether the field is a textarea with summary, fetch the summary, if not -
    // fetch it's safe value.
    if (!empty($teaser_field) && isset($teaser_field['#items'][0]['safe_summary'])) {
      $teaser = $teaser_field['#items'][0]['safe_summary'];
    }
    elseif (isset($teaser_field['#items'][0]['safe_value'])) {
      $teaser = $teaser_field['#items'][0]['safe_value'];
    }

    if (!empty($body_field) && isset($body_field['#items'][0]['safe_value'])) {
      $body = $body_field['#items'][0]['safe_value'];
    }

    // Empty the teaser, if body and teaser are mapped to same fields, and the values
    // are identical.
    if ($teaser_map == $body_map && md5($teaser) == md5($body)) {
      $teaser  = '';
    }

    $bpi_content['body'] = $body;
    $bpi_content['teaser'] = $teaser;
    $dt = new DateTime();
    $dt->setTimestamp($node->changed);
    $bpi_content['creation'] = $dt->format(DateTime::W3C);
    $bpi_content['type'] = $node->type;
    $bpi_content['category'] = $category;
    $bpi_content['audience'] = $audience;
    $bpi_content['editable'] = 0;
    $bpi_content['authorship'] = 0;
  }

  return $bpi_content;
}

/**
 * Fetch dictionaries from BPI ws.
 *
 * @return array
 *   Nested array of dictionaries, keyed by dictionary
 *   type.
 */
function bpi_get_bpi_dictionaries() {
  $dictionaries = array();
  $endpoint = variable_get('bpi_service_url', '');
  $agency_id = variable_get('bpi_agency_id', '');
  $api_key = variable_get('bpi_api_key', '');
  $secret_key = variable_get('bpi_secret_key', '');

  try {
    $bpi = new Bpi($endpoint, $agency_id, $api_key, $secret_key);
    $dictionaries = $bpi->getDictionaries();
  }
  catch (Exception $e) {
    watchdog(
      'bpi',
      t('Failed to fetch dictionaries, with message %message.'),
      array(
        '%message' => $e->getMessage()
      ),
      WATCHDOG_ERROR
    );
  }

  return $dictionaries;
}