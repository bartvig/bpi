<?php
/**
 * @file
 * bpi_features.field_group.inc
 */

/**
 * Implements hook_field_group_info().
 */
function bpi_features_field_group_info() {
  $export = array();

  $field_group = new stdClass();
  $field_group->disabled = FALSE; /* Edit this to true to make a default field_group disabled initially */
  $field_group->api_version = 1;
  $field_group->identifier = 'group_workflow|node|ding_news|form';
  $field_group->group_name = 'group_workflow';
  $field_group->entity_type = 'node';
  $field_group->bundle = 'ding_news';
  $field_group->mode = 'form';
  $field_group->parent_name = '';
  $field_group->data = array(
    'label' => 'Workflow',
    'weight' => '36',
    'children' => array(
      0 => 'field_bpi_workflow',
      1 => 'field_bpi_id',
      2 => 'group_bpi_vocabularies',
      3 => 'group_bpi_options',
    ),
    'format_type' => 'tab',
    'format_settings' => array(
      'label' => 'Workflow',
      'instance_settings' => array(
        'required_fields' => 0,
        'classes' => '',
        'description' => '',
      ),
      'formatter' => 'closed',
    ),
  );
  $export['group_workflow|node|ding_news|form'] = $field_group;

  return $export;
}