<?php
/**
 * @file forena.datablock.inc
 * Data block building and testing forms.
 */
require_once 'FrxDataSource.inc';
function forena_query_builder_form($formid, &$form_state) {
  // Define an empty block
  $access = TRUE;
  $default_storage = array(
    'block' => array(),
    'block_name' => '',
    'parameters' => array(),
    'provider' => '',
    'builder' => array(
      'columns' => array(),
      'block' => '',
      'where' => array(
        'op' => 'AND',
        'filter' => array(
          array('field' => '', 'op' => '', 'value' => ''),
        ),
      ),
      'group' => array(),
      'order' => array(),
      ),
    'fields' => array(),
    'step' => 'block',
    'preview' => '',
    'new' => TRUE,
  );

  // Work with the existing block or load default
  if (!isset($form_state['storage']))  {
    $form_state['storage'] = $default_storage;

  }
  $data = &$form_state['storage'];
  $step = $form_state['storage']['step'];

  /**
   * Block select portion of ofrm
   */
  // First stage is to either load an existing block or create a new one
  if ($step == 'block') {

    $form['block'] = array(
      '#type' => 'textfield',
      '#autocomplete_path' => 'forena/data_block/autocomplete',
      '#title' => 'Data Block',
      '#required' => TRUE,
      '#description' => t('Select an exising block or create a new one derived from the selected block.'),
    );

    $form['load_include'] = array(
      '#type' => 'submit',
      '#submit' => array('forena_query_load_include', 'forena_query_preview'),
      '#value' => t('New'),
    );

    $form['load'] = array(
      '#type' => 'submit',
      '#submit' => array('forena_query_load_block', 'forena_query_preview'),
      '#value' => t('Load'),
    );

  }
  // If we're not on the first form than we need to verify the acces for the block
  else {
     $access = forena_query_access_repository($form_state['storage']['block_name']);
     /**
     * Section of form for finally saving the report.
     */
    if ($access && $form_state['storage']['new']) {
      $form['save_as_name'] = array(
        '#type' => 'textfield',
        '#title' => t('Name') . ' ' . $form_state['storage']['provider'] . '/',
        '#description' => t('Name of block to save'),
      );
    }

    if (@$form_state['storage']['delete']) {
      $form['question'] = array(
        '#type' => 'markup',
        '#markup' => '<p>Are you sure you want to delete ' . $data['block_name'] . '?</p>',
      );

      $form['confirm'] = array(
        '#type' => 'submit',
        '#submit' => array('forena_query_delete_block'),
        '#value' => t('Yes'),
      );
      $form['cancel'] = array(
        '#type' => 'submit',
        '#submit' => array('forena_query_delete_cancel'),
        '#value' => t('No'),
      );
      return $form;
    }

    /**
     * Edit query builder clause portion of form.
     */
    if ($step == 'builder') {
      $form['where'] = array('#tree' => TRUE);

      $form['where'] = array(
        '#type' => 'fieldset',
        '#title' => 'Filter Criteria',
        '#tree' => TRUE,
      );
      _forena_query_filter_ctl($data['builder']['where'], $form['where'], $form_state['storage']['fields']);
      $form['sql'] = array(
        '#type' => 'submit',
        '#value' => t('SQL'),
        '#submit' => array('forena_query_update_builder', 'forena_query_sql_view'),
      );
    }

    /**
     * Edit SQL directly.
     */
    if ($step == 'sql') {
      $block = $data['block'];
      if ($block && $block['type']=='sql') {
        $form['file'] = array(
          '#type' => 'textarea',
          '#title' => t('Source'),
          '#disabled' => @$block['locked'],
          '#default_value' => @$block['file'],
        );

        if (@$block['builder']) {
          $form['builder_view'] = array(
            '#type' => 'submit',
            '#value' => t('Query Builder'),
            '#submit' => array('forena_query_builder_view'),
          );
        }


      }


    }

    if (@$block['tokens']) {
      $form['parameters'] = array(
        '#title' => 'Parameters',
        '#type' => 'fieldset',
        '#tree' => TRUE
      );

      foreach ($block['tokens'] as $name) {
        $ctl = array(
          '#type' => 'textfield',
          '#title' => check_plain($name),
        );
        $form['parameters'][$name] = $ctl;
      }
    }

    if (!@$form_state['storage']['new'] && $step != $block) {
      $form['block'] = array(
        '#type' => 'value',
        '#value' => $data['block_name'],
      );

      $form['load'] = array(
        '#type' => 'submit',
        '#submit' => array('forena_query_load_block', 'forena_query_preview'),
        '#value' => t('Reload'),
        );

      }

    if (!@$block['locked'] && $access) {
      $form['save'] = array(
        '#type' => 'submit',
        '#value' => 'Save',
        '#validate' => array('forena_query_validate_new_block'),
        '#submit' => array('forena_query_update', 'forena_query_save_block')
      );

      if (!$data['new']) $form['delete'] = array('#type' => 'submit', '#value' => t('Delete'), '#submit' => array( 'forena_query_try_delete'));
    }
    $form['submit'] = array('#type' => 'submit', '#value' => 'Preview', '#submit' => array('forena_query_update', 'forena_query_preview'));
    $form['cancel'] = array('#type' => 'submit', '#value' => 'Close', '#submit' => array('forena_query_close'));
    $form['preview'] = array('#markup' => @$form_state['storage']['preview']);
  }

  return $form;

}

function forena_query_load_block($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $form_state['storage']['new'] = FALSE;
  $form_state['storage']['block'] = $block_def = Frx::RepoMan()->loadBlock($form_state['values']['block']);
  list($provider, $block) = @explode('/', $form_state['values']['block'], 2);
  $form_state['storage']['provider'] = $provider;
  $form_state['storage']['block_name'] = $form_state['values']['block'];
  if (@$block_def['builder']) {
    $form_state['storage']['builder'] = $block_def['builder'];
    $form_state['storage']['step'] = 'builder';
  }
  else {
    $form_state['storage']['step'] =  'sql';
  }

}

/**
 * Load include block as source of builder.
 * @param $form
 * @param $form_state
 */
function forena_query_load_include($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $form_state['storage']['block'] = Frx::RepoMan()->loadBlock($form_state['values']['block']);
  $form_state['storage']['block']['locked'] = FALSE;
  list($provider, $block) = @explode('/', $form_state['values']['block'], 2);
  $form_state['storage']['provider'] = $provider;
  $form_state['storage']['builder']['block_name'] = $block;
  $form_state['storage']['block_name'] = $form_state['values']['block'];
  $form_state['storage']['new'] = TRUE;
  $form_state['storage']['step'] = 'builder';
  $form_state['input']['file'] = $form_state['values']['file'] = @$form_state['storage']['block']['file'];
}

/**
 * Submit handler to update the form fields.
 */
function forena_query_update_sql($form, &$form_state) {
  $file = $form_state['values']['file'];
  $provider = $form_state['storage']['provider'];
  $block_def = $form_state['storage']['block'];
  if ($block_def['file']!=$file) {
    $block_def['builder'] = '';
    $new_block = Frx::RepoMan()->sqlBlock($provider, $file);
    if ($new_block) $block_def = array_merge($block_def, $new_block);
    $form_state['storage']['block'] = $block_def;
  }
}

function forena_query_preview($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $provider = $form_state['storage']['provider'];
  $block_def = $form_state['storage']['block'];
  $parms = $form_state['storage']['parameters'];
  if (isset($form_state['values']['parameters'])) {
    $parms = $form_state['storage']['parameters'] = $form_state['values']['parameters'];
  }
  FrxReportGenerator::instance()->alter_parameters('', $parms);
  if ($block_def['type'] == 'sql') {
    $sql = $block_def['file'];
    $data = Frx::RepoMan()->sqlData($provider, $sql, $parms);
  }
  else {
    $block_name = $form_state['storage']['block_name'];
    $data = Frx::RepoMan()->data($block_name);
  }
  $t = Frx::Template('FrxTable');
  $config = array('class' => 'dataTable', 'id' => 'preview-data');
  if ($data && $t) {
    $t->generate($data, $config);
    $fields = $t->columns;
    $form_state['storage']['fields'] = array_combine($fields, $fields);

    $report = $t->asXML();
    $rpt = new FrxReport($report, $data);
    $form_state['storage']['preview'] = $rpt->render('web');
  }
  else {
    $form_state['storage']['preview'] = '';
  }
  if (is_object($data)) $data = $data->asXML();

}

function forena_query_validate_new_block($form, &$form_state) {

  // Check to make sure a filename is specified.
  if (isset($form_state['values']['save_as_name'])) {
    $block_name = $form_state['storage']['provider'] . '/' . $form_state['values']['save_as_name'];

    $block = Frx::RepoMan()->loadBlock($block_name);
    if ($block) {
      form_set_error('save_as_name', t('Block already exists'));
      return;
    }
  }
}

function forena_query_save_block($form, &$form_state) {
  $form_state['rebuild'] = FALSE;
  $block_name = $form_state['storage']['block_name'];
  if (isset($form_state['values']['save_as_name'])) {
    $block_name = $form_state['storage']['provider'] . '/' . trim($form_state['values']['save_as_name']);
  }
  $data = $form_state['storage']['block'];
  Frx::RepoMan()->saveBlock($block_name, $data);
  drupal_set_message($block_name . ' saved');
}

function forena_query_delete_block($form, &$form_state) {
  $block_name = $form_state['storage']['block_name'];
  Frx::RepoMan()->deleteBlock($block_name);
  $form_state['rebuild'] = FALSE;
  drupal_set_message(t('Deleted %s', array('%s' => $block_name)));
}

function forena_query_close($form, &$form_state) {
  $form_state['rebuild'] = FALSE;
}

function forena_query_sql_view($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $form_state['storage']['step'] = 'sql';
}

function forena_query_builder_view($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $form_state['storage']['step'] = 'builder';
}

function forena_query_delete_cancel($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $form_state['storage']['delete'] = FALSE;
}

function forena_query_try_delete($form, &$form_state) {
  $form_state['storage']['delete'] = TRUE;
  $form_state['rebuild'] = TRUE;
}


/**
 * Recursively clean all submit values from form elements
 * assuming tree = true structure.
 * @param $values
 */
function _forena_query_clean_values(&$values, &$elements) {
  foreach ($elements as $key => $value) if (strpos($key, '#')!==0) {
    if (@$value['#type']=='submit') {
      unset($values[$key]);
    }
    else {
      _forena_query_clean_values($values[$key], $elements[$key]);
    }
  }
}


/**
 * Implements query builder form.
 * @param $formid
 * @param $form_state
 */

/**
 * Recursively build query form.
 * @param $data array of data where clause filters
 * @param $ctl form control to modify
 * @param unknown_type $fields
 */
function _forena_query_filter_ctl(&$data, &$ctl, $fields) {
  static $i = 0;
  $i++;
  $operators = array(
    '=' => 'is',
    '<>' => 'is not',
    '>' => 'is greater than',
    '<' => 'is less than',
    'LIKE' => 'is like',
    'IS NOT NULL' => 'is not null',
    'IS NULL' => 'is null');
  $ctl['op'] = array(
    '#type' => 'select',
    '#options' => array('AND' => 'all of', 'OR' => 'any of' ),
    '#default_value' => $data['op'],
  );
  $ctl['filter'] = array('#type' => 'fieldset');
  if ($i>1) $ctl['filter']['ungroup'] = array(
    '#type' => 'submit',
    '#value' => t('Remove'),
    '#name' => $i,
    '#submit' => array('forena_query_remove_group'),
    '#weight' => 1);

  $i++;
  foreach ($data['filter'] as $k => $crit) {
    $i++;
    if (isset($crit['filter'])) {
      $cnd = array();
      _forena_query_filter_ctl($data['filter'][$k], $cnd, $fields);
    }
    else {
      $cnd = array('#prefix' => '<div class="forena-filter">', '#suffix' => '</div>');
      $cnd['field'] = array('#type' => 'select', '#options' => $fields, '#default_value' => $crit['field']);
      $cnd['op'] = array('#type' => 'select', '#options' => $operators, '#default_value' => $crit['op']);
      $cnd['value'] = array('#type' => 'textfield', '#default_value' => $crit['value'] );
      $cnd['group'] = array('#type' => 'submit', '#value' => '( )', '#submit' => array('forena_query_group_filter'), '#name' => $i);
      $i++;
      if ($k>0) $cnd['remove_filter'] = array('#type' => 'submit', '#value' => '-', '#submit' => array('forena_query_remove_filter'), '#name' => $i);
      $i++;
    }
    $ctl['filter'][]  = $cnd;
  }
  $i++;
  $ctl['filter']['add_filter'] = array('#type' => 'submit', '#value' => '+', '#submit' => array('forena_query_add_filter'), '#name' => $i);
}

/**
 * Function to allow all normal button submits to be for the form.
 * @param unknown_type $form
 * @param unknown_type $form_state
 */
function forena_query_update($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  switch ($form_state['storage']['step']) {
    case 'sql':
      forena_query_update_sql($form, $form_state);
      break;
    case 'builder':
      forena_query_update_builder($form, $form_state);
      break;
  }
}

function forena_query_update_builder($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $values = $form_state['values'];
  _forena_query_clean_values($values['where'], $form['where']);
  $form_state['storage']['builder']['where'] = $values['where'];
  $provider = $form_state['storage']['provider'];
  $sql = Frx::RepoMan()->buildSQL($provider, $form_state['storage']['builder']);
  $form_state['storage']['block']['file'] = $sql;
  $form_state['storage']['block']['builder'] = $form_state['storage']['builder'];
}


/**
 * Assuming a tree=true structure, drill down into parent to
 * add a data structure to the appropriate point in the tree.
 * @param $values data from form elements
 * @param $parents array of parents that indicates how deep to set the tree.
 * @param $to_add element to add
 */
function _forena_query_add_parent_data(&$values, &$parents, $to_add) {
  if ($parents) {
    $key = array_shift($parents);
    _forena_query_add_parent_data($values[$key], $parents, $to_add);
  }
  else {
    $values[]=$to_add;
  }
}

/**
 * Add another field to the filter.
 * @param unknown_type $form
 * @param unknown_type $form_state
 */
function forena_query_add_filter($form, &$form_state) {
  $where = $form_state['values']['where'];
  _forena_query_clean_values($where, $form['where']);
  // Determine containing field of pressed button.
  $parents = $form_state['triggering_element']['#parents'];
  array_shift($parents);
  array_pop($parents);
  $field = array_fill_keys(array('field', 'op', 'value'), '');
  _forena_query_add_parent_data($where, $parents, $field);
  $form_state['storage']['builder']['where'] = $where;
  $form_state['rebuild'] = TRUE;

}

/**
 * Assuming a tree=true structure, drill down into parent to
 * add a data structure to the appropriate point in the tree.
 * @param $values data from form elements
 * @param $parents array of parents that indicates how deep to set the tree.
 * @param $to_add element to add
 */
function _forena_query_remove_parent_data(&$values, &$parents) {
   if ($parents) {
    $key = array_shift($parents);
    if (!$parents) {
      unset($values[$key]);
    }
    else {
      _forena_query_remove_parent_data($values[$key], $parents);
    }
  }
}

/**
 *
 * Remove a filter grouping.
 * @param unknown_type $form
 * @param unknown_type $form_state
 */
function forena_query_remove_filter($form, &$form_state) {
  $where = $form_state['values']['where'];
  _forena_query_clean_values($where, $form['where']);
  // Determine containing field of pressed button.
  $parents = $form_state['triggering_element']['#parents'];
  array_shift($parents);
  array_pop($parents);
  _forena_query_remove_parent_data($where, $parents);
  $form_state['storage']['builder']['where'] = $where;
  $form_state['rebuild'] = TRUE;
}

/**
 *
 * Remove a grouping.
 * @param unknown_type $form
 * @param unknown_type $form_state
 */
function forena_query_remove_group($form, &$form_state) {
  $where = $form_state['values']['where'];
  _forena_query_clean_values($where, $form['where']);
  // Determine containing field of pressed button.
  $parents = $form_state['triggering_element']['#parents'];
  array_shift($parents);
  array_pop($parents);
  array_pop($parents);
  _forena_query_remove_parent_data($where, $parents);
  if (!$where['filter']) {
    $where['filter'][] =  array_fill_keys(array('field', 'op', 'value'), '');
  }
  $form_state['storage']['builder']['where'] = $where;
  $form_state['rebuild'] = TRUE;
}

function _forena_query_group_parent_data(&$values, &$parents) {
  if ($parents) {
    $key = array_shift($parents);
    _forena_query_group_parent_data($values[$key], $parents);
  }
  else {
    $filter = $values;
    $values = array(
      'op' => 'AND',
      'filter' => array(),
    );
    $values['filter'][] = $filter;
  }
}


/**
 * Group a filter
 * @param unknown_type $form
 * @param unknown_type $form_state
 */
function forena_query_group_filter($form, &$form_state) {
   $where = $form_state['values']['where'];
  _forena_query_clean_values($where, $form['where']);
  // Determine containing field of pressed button.
  $parents = $form_state['triggering_element']['#parents'];
  array_shift($parents);
  array_pop($parents);
  _forena_query_group_parent_data($where, $parents);
  $form_state['storage']['builder']['where'] = $where;
  $form_state['rebuild'] = TRUE;
}

/**
 * Submit handler to validate the next step in the query builder
 */
function forena_query_builder_next($form, &$form_state) {
  $next = $form_state['values']['next'];
  $data = $form_state['storage'];
  $step = 'block';
  switch ($next) {
    case 'where':
      if ($data['preview'] && $data['block_name']) {
        $step = $next;
      }
      break;
    default:
      $step = 'block';
  }
  $form_state['storage']['step'] = $step;
}

/**
 * Build SQL query from data
 *
 */
function forena_query_build_sql($form, &$form_state) {
  $builder = $formstate['storage']['builder'];
  foreach ($builder as $filter) {

  }
}



