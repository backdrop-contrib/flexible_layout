<?php


/**
 * Create a renderer object.
 *
 * The renderer object contains data that is passed around from function
 * to function allowing us to render our CSS and HTML easily.
 *
 * @todo Convert the functions to methods and make this properly OO.
 */
function flexible_layout_create_renderer($admin, $content, $settings, $layout, $handler) {
  $renderer = new stdClass;
  $renderer->admin = TRUE; // this must go
  $renderer->settings = $settings;
  $renderer->content = $content;
  $renderer->did = '';
  if ($admin) {
    // always scale in admin mode.
    $renderer->scale_base = 99.0;
  }
  else {
    $renderer->scale_base = !empty($settings['items']['canvas']['no_scale']) ? 100.0 : 99.0;
  }
  $renderer->id_str = '';
  $renderer->admin = $admin;
  $renderer->handler = $handler;

  // Set up basic classes for all of our components.
  $renderer->name                 = !empty($layout->layout_template) ? $layout->layout_template : '';
  $renderer->base_class           = $renderer->name;
  $renderer->item_class['column'] = 'flexible-layout-column';
  $renderer->item_class['row']    = 'flexible-layout-row';
  $renderer->item_class['region'] = 'flexible-layout-region';
  $renderer->base['canvas']       = 'flexible-layout-' . $renderer->base_class;

  // Override these if selected from the UI and not in admin mode.
  if (!$admin) {
    if (!empty($settings['items']['canvas']['class'])) {
      $renderer->base_class = $settings['items']['canvas']['class'];
      $renderer->base['canvas'] = $renderer->base_class;
    }
    if (!empty($settings['items']['canvas']['column_class'])) {
      $renderer->item_class['column'] = $settings['items']['canvas']['column_class'];
    }
    if (!empty($settings['items']['canvas']['row_class'])) {
      $renderer->item_class['row'] = $settings['items']['canvas']['row_class'];
    }
    if (!empty($settings['items']['canvas']['region_class'])) {
      $renderer->item_class['region'] = $settings['items']['canvas']['region_class'];
    }
  }

  // Get the separation values out of the canvas settings.
  $renderer->column_separation = !empty($settings['items']['canvas']['column_separation']) ? $settings['items']['canvas']['column_separation'] : '0.5em';

  $renderer->region_separation = !empty($settings['items']['canvas']['region_separation']) ? $settings['items']['canvas']['region_separation'] : '0.5em';
  $renderer->row_separation = !empty($settings['items']['canvas']['row_separation']) ? $settings['items']['canvas']['row_separation'] : '0.5em';

  // Make some appended classes so it's easier to reference them.
  $renderer->base['column'] = $renderer->item_class['column'] . '-' . $renderer->base_class;
  $renderer->base['row']    = $renderer->item_class['row'] . '-' . $renderer->base_class;
  $renderer->base['region'] = $renderer->item_class['region'] . '-' . $renderer->base_class;
  if ($renderer->name != 'new') {
    // Use v2 to guarantee all CSS gets regenerated to account for changes in
    // how some divs will be rendered.
    $renderer->css_cache_name = 'flexiblev2:' . $renderer->name;
    if ($admin) {
    }
  }
  return $renderer;
}

/**
 * Draw the flexible layout.
 */
/**
 * Draw the flexible layout.
 */
function flexible_layout_preprocess_layout(&$vars) {
  $vars['output'] = '';
  if ($vars['layout_info']['name'] == 'flexible_template' && empty($vars['admin'])) {
    $content = array();
    $settings = $vars['settings'];
    $layout = $vars['layout'];
    $handler = $vars['renderer'];

    // Build rendrer content
    $layout_positions = array_filter($layout->positions);
    foreach ($layout_positions as $region => $blocks) {
      $content[$region] = '';
      foreach ($blocks as $uuid) {
        $content[$region] .= $handler->rendered['blocks'][$uuid];
      }
    }
    $renderer = flexible_layout_create_renderer(FALSE, $content, $settings['flexible'], $layout, $handler);

    // CSS must be generated because it reports back left/middle/right
    // positions.
    $css = flexible_layout_render_css($renderer);

    // Todo: this was cached in D7 Panels.
    backdrop_add_css($css, array('type' => 'inline', 'preprocess' => FALSE));

    $output = "<div class=\"panel-flexible " . $renderer->base['canvas'] . " clearfix\" $renderer->id_str>\n";
    $output .= "<div class=\"panel-flexible-inside " . $renderer->base['canvas'] . "-inside\">\n";

    $output .= flexible_layout_render_items($renderer, $settings['flexible']['items']['canvas']['children'], $renderer->base['canvas']);

    // Wrap the whole thing up nice and snug
    $output .= "</div>\n</div>\n";
    $vars['output'] = $output;

  }
}

/**
 * Render a piece of a flexible layout.
 */
function flexible_layout_render_items($renderer, $list, $owner_id) {
  $output = '';
  $groups = array('left' => '', 'middle' => '', 'right' => '');
  $max = count($list) - 1;
  $prev = NULL;

  foreach ($list as $position => $id) {
    $item = $renderer->settings['items'][$id];
    $location = isset($renderer->positions[$id]) ? $renderer->positions[$id] : 'middle';

    if ($renderer->admin && $item['type'] != 'row' && $prev ) {
      $groups[$location] .= flexible_layout_render_splitter($renderer, $prev, $id);
    }

    switch ($item['type']) {
      case 'column':
        $content = flexible_layout_render_items($renderer, $item['children'], $renderer->base['column'] . '-' . $id);
        if (empty($renderer->settings['items'][$id]['hide_empty']) || trim($content)) {
          $groups[$location] .= flexible_layout_render_item($renderer, $item, $content, $id, $position, $max);
        }
        break;
      case 'row':
        $content = flexible_layout_render_items($renderer, $item['children'], $renderer->base['row'] . '-' . $id);
        if (empty($renderer->settings['items'][$id]['hide_empty']) || trim($content)) {
          $groups[$location] .= flexible_layout_render_item($renderer, $item, $content, $id, $position, $max, TRUE);
        }
        break;
      case 'region':
        if (empty($renderer->settings['items'][$id]['hide_empty'])) {
          $content = isset($renderer->content[$id]) ? $renderer->content[$id] : "&nbsp;";
        }
        else {
          $content = isset($renderer->content[$id]) ? trim($renderer->content[$id]) : "";
        }
        if (empty($renderer->settings['items'][$id]['hide_empty']) || $content) {
          $groups[$location] .= flexible_layout_render_item($renderer, $item, $content, $id, $position, $max);
        }
        break;
    }

    // If all items are fixed then we have a special splitter on the right to
    // control the overall width.
    if (!empty($renderer->admin) && $max == $position && $location == 'left') {
      $groups[$location] .= flexible_layout_render_splitter($renderer, $id, NULL);
    }
    $prev = $id;
  }

  $group_count = count(array_filter($groups));

  // Render each group. We only render the group div if we're in admin mode
  // or if there are multiple groups.
  foreach ($groups as $position => $content) {
    if (!empty($content) || $renderer->admin) {
      if ($group_count > 1 || $renderer->admin) {
        $output .= '<div class="' . $owner_id . '-' . $position . '">' . $content . '</div>';
      }
      else {
        $output .= $content;
      }
    }
  }

  return $output;
}

/**
 * Render a column in the flexible layout.
 */
function flexible_layout_render_item($renderer, $item, $content, $id, $position, $max, $clear = FALSE) {

  // If we are rendering a row and there is just one row, we don't need to
  // render the row unless there is fixed_width content inside it.
  if (empty($renderer->admin) && $item['type'] == 'row' && $max == 0) {
    $fixed = FALSE;
    foreach ($item['children'] as $id) {
      if ($renderer->settings['items'][$id]['width_type'] != '%') {
        $fixed = TRUE;
        break;
      }
    }

    if (!$fixed) {
      return $content;
    }
  }

  // If we are rendering a column and there is just one column, we don't
  // need to render the column unless it has a fixed_width.
  if (empty($renderer->admin) && $item['type'] == 'column' && $max == 0 && $item['width_type'] == '%') {
    return $content;
  }

  $base = $renderer->item_class[$item['type']];
  $output = '<div class="' . $base . ' ' . $renderer->base[$item['type']] . '-' . $id;
  if ($position == 0) {
    $output .= ' ' . $base . '-first';
  }
  if ($position == $max) {
    $output .= ' ' . $base . '-last';
  }
  if ($clear) {
    $output .= ' clearfix';
  }

  if (isset($item['class'])) {
    $output .= ' ' . check_plain($item['class']);
  }

  $output .= '">' . "\n";

  if (!empty($renderer->admin)) {
    $output .= flexible_layout_render_item_links($renderer, $id, $item);
  }

  $output .= '  <div class="inside ' . $base . '-inside ' . $base . '-' . $renderer->base_class . '-' . $id . '-inside';
  if ($position == 0) {
    $output .= ' ' . $base . '-inside-first';
  }
  if ($position == $max) {
    $output .= ' ' . $base . '-inside-last';
  }
  if ($clear) {
    $output .= ' clearfix';
  }

  $output .= "\">\n";
  $output .= $content;
  $output .= '  </div>' . "\n";
  $output .= '</div>' . "\n";

  return $output;
}

/**
 * Render a splitter div to place between the $left and $right items.
 *
 * If the right ID is NULL that means there isn't actually a box to the
 * right, but we need a splitter anyway. We'll mostly use info about the
 * left, but pretend it's 'fluid' so that the javascript won't actually
 * modify the right item.
 */
function flexible_layout_render_splitter($renderer, $left_id, $right_id) {
  $left = $renderer->settings['items'][$left_id];

  $left_class = $renderer->base[$left['type']] . '-' . $left_id;
  if ($right_id) {
    $right = $renderer->settings['items'][$right_id];
    $right_class = $renderer->base[$left['type']] . '-' . $right_id;
  }
  else {
    $right = $left;
    $right_class = $left_class;
  }

  $output = '<div tabindex="0"
    class="flexible-layout-splitter flexible-splitter-for-' . $left_class . '">';

  // Name the left object
  $output .= '<span class="flexible-layout-splitter-left">';
  $output .= '.' . $left_class;
  $output .= '</span>';

  $output .= '<span class="flexible-layout-splitter-left-id">';
  $output .= $left_id;
  $output .= '</span>';

  $output .= '<span class="flexible-layout-splitter-left-width ' . $left_class . '-width">';
  $output .= $left['width'];
  $output .= '</span>';

  $output .= '<span class="flexible-layout-splitter-left-scale">';
  $output .= isset($renderer->scale[$left_id]) ? $renderer->scale[$left_id] : 1;
  $output .= '</span>';

  $output .= '<span class="flexible-layout-splitter-left-width-type">';
  $output .= $left['width_type'];
  $output .= '</span>';

  // Name the right object
  $output .= '<span class="flexible-layout-splitter-right">';
  $output .= '.' . $right_class;
  $output .= '</span>';

  $output .= '<span class="flexible-layout-splitter-right-id">';
  $output .= $right_id;
  $output .= '</span>';

  $output .= '<span class="flexible-layout-splitter-right-width ' . $right_class . '-width">';
  $output .= $right['width'];
  $output .= '</span>';

  $output .= '<span class="flexible-layout-splitter-right-scale">';
  $output .= isset($renderer->scale[$right_id]) ? $renderer->scale[$right_id] : 1;
  $output .= '</span>';

  $output .= '<span class="flexible-layout-splitter-right-width-type">';
  // If there is no right, make it fluid.
  $output .= $right_id ? $right['width_type'] : '%';
  $output .= '</span>';

  $output .= '</div>';
  return $output;
}

/**
 * Render the dropdown links for an item.
 */
function flexible_layout_render_item_links($renderer, $id, $item) {
  $links = array();
  $remove = '';
  $add = '';
  if ($item['type'] == 'column') {
    $title = t('Column');
    $settings = t('Column settings');
    if (empty($item['children'])) {
      $remove = t('Remove column');
      $add = t('Add row');
    }
    else {
      $add = t('Add row to top');
      $add2 = t('Add row to bottom');
    }
  }
  else if ($item['type'] == 'row') {
    if ($id == 'canvas') {
      $title = t('Canvas');
      $settings = t('Canvas settings');
    }
    else {
      $title = t('Row');
      $settings = t('Row settings');
    }
    if (empty($item['children'])) {
      if ($id != 'canvas') {
        $remove = t('Remove row');
      }
      $add = $item['contains'] == 'region' ? t('Add region') : t('Add column');
    }
    else {
      $add = $item['contains'] == 'region' ? t('Add region to left') : t('Add column to left');
      $add2 = $item['contains'] == 'region' ? t('Add region to right') : t('Add column to right');
    }
  }
  else if ($item['type'] == 'region') {
    $title = t('Region');
    $settings = t('Region settings');
    $remove = t('Remove region');
  }

  if (!empty($settings)) {
    $links[] = array(
      'title' => $settings,
      'href' => $renderer->handler->getUrl('flexible-layout', 'settings', $id),
      'attributes' => array(
        'class' => array('use-ajax'),
        'data-dialog' => TRUE,
        'data-dialog-options' => json_encode(array('dialogClass' => 'layout-dialog')),
      ),
    );
  }
  if ($add) {
    $links[] = array(
      'title' => $add,
      'href' => $renderer->handler->getUrl('flexible-layout', 'add', $id),
      'attributes' => array(
        'class' => array('use-ajax'),
        'data-dialog' => TRUE,
        'data-dialog-options' => json_encode(array('dialogClass' => 'layout-dialog')),
      ),
    );
  }
  if (isset($add2)) {
    $links[] = array(
      'title' => $add2,
      'href' => $renderer->handler->getUrl('flexible-layout', 'add', $id, 'right'),
      'attributes' => array(
        'class' => array('use-ajax'),
        'data-dialog' => TRUE,
        'data-dialog-options' => json_encode(array('dialogClass' => 'layout-dialog')),
      ),
    );
  }
  if ($remove) {
    $links[] = array(
      'title' => $remove,
      'href' => $renderer->handler->getUrl('flexible-layout', 'remove', $id),
      'attributes' => array(
        'class' => array('use-ajax'),
        'data-dialog' => FALSE,
      ),
    );
  }
  $operations = array(
    '#type' => 'dropbutton',
    '#links' => $links,
  );
  return backdrop_render($operations);
}
/**
 * Provide CSS for a flexible layout.
 */
function flexible_layout_render_css($renderer) {
  if ($renderer->admin) {
    $parent_class = '.' . $renderer->base['row'] . '-canvas';
  }
  else {
    $parent_class = '.' . $renderer->base['canvas'];
  }
  return flexible_layout_render_css_group($renderer, $renderer->settings['items']['canvas']['children'], $parent_class, 'column', 'canvas');
}

/**
 * Render the CSS for a group of items to be displayed together.
 *
 * Columns and regions, when displayed as a group, need to cooperate in
 * order to share margins and make sure that percent widths add up
 * to the right total.
 */
function flexible_layout_render_css_group($renderer, $list, $owner_id, $type, $id) {
  $css = array();

  // Start off with some generic CSS to properly pad regions
  $css[$owner_id . ' .' . $renderer->item_class['region']] = array(
    'padding' => '0',
  );

  $css[$owner_id . ' .' . $renderer->item_class['region'] . '-inside'] = array(
    'padding-right' => $renderer->region_separation,
    'padding-left' => $renderer->region_separation,
  );

  $css[$owner_id . ' .' . $renderer->item_class['region'] . '-inside-first'] = array(
    'padding-left' => '0',
  );

  $css[$owner_id . ' .' . $renderer->item_class['region'] . '-inside-last'] = array(
    'padding-right' => '0',
  );
  $css[$owner_id . ' .' . $renderer->item_class['column']] = array(
    'padding' => '0',
  );

  $css[$owner_id . ' .' . $renderer->item_class['column'] . '-inside'] = array(
    'padding-right' => $renderer->column_separation,
    'padding-left' => $renderer->column_separation,
  );

  $css[$owner_id . ' .' . $renderer->item_class['column'] . '-inside-first'] = array(
    'padding-left' => '0',
  );

  $css[$owner_id . ' .' . $renderer->item_class['column'] . '-inside-last'] = array(
    'padding-right' => '0',
  );

  // And properly pad rows too
  $css[$owner_id . ' .' . $renderer->item_class['row']] = array(
    'padding' => '0 0 ' . $renderer->row_separation . ' 0',
    'margin' => '0',
  );

  $css[$owner_id . ' .' . $renderer->item_class['row'] . '-last'] = array(
    'padding-bottom' => '0',
  );

  flexible_layout_get_css_group($css, $renderer, $list, $owner_id, $type, $id);

  return ctools_css_assemble($css);
  // return '';
}

function ctools_css_assemble($css_data) {
  // Initialize the output.
  $css = '';
  // Iterate through all the statements.
  foreach ($css_data as $selector_str => $declaration) {
    // Add the selectors, separating them with commas and line feeds.
    $css .= strpos($selector_str, ',') === FALSE ? $selector_str : str_replace(", ", ",\n", $selector_str);
    // Add the opening curly brace.
    $css .= " {\n";
    // Iterate through all the declarations.
    foreach ($declaration as $property => $value) {
      $css .= "  " . $property . ": " . $value . ";\n";
    }
    // Add the closing curly brace.
    $css .= "}\n\n";
  }
  // Return the output.
  return $css;
}

/**
 * Construct an array with all of the CSS properties for a group.
 *
 * This will parse down into children and produce all of the CSS needed if you
 * start from the top.
 */
function flexible_layout_get_css_group(&$css, $renderer, $list, $owner_id, $type, $item_id) {
  if ($type != 'row') {
    // Go through our items and break up into right/center/right groups so we
    // can figure out our offsets.

    // right == any items on the right that are 'fixed'.
    // middle == all fluid items.
    // right == any items on the right that are 'fixed'.
    $left = $middle = $right = array();
    $left_total = $right_total = $middle_total = 0;
    $current = 'left';
    foreach ($list as $id) {
      if ($renderer->settings['items'][$id]['width_type'] == 'px') {
        // fixed
        if ($current == 'left') {
          $left[] = $id;
          $renderer->positions[$id] = 'left';
          $left_total += $renderer->settings['items'][$id]['width'];
        }
        else {
          $current = 'right';
          $right[] = $id;
          $renderer->positions[$id] = 'right';
          $right_total += $renderer->settings['items'][$id]['width'];
        }
      }
      else {
        // fluid
        if ($current != 'right') {
          $current = 'middle';
          $middle[] = $id;
          $renderer->positions[$id] = 'middle';
          $middle_total += $renderer->settings['items'][$id]['width'];
        }
        // fall through: if current is 'right' and we ran into a 'fluid' then
        // it gets *dropped* because that is invalid.
      }
    }

    // Go through our right sides and create CSS.
    foreach ($left as $id) {
      $class = "." . $renderer->base[$type] . "-$id";
      $css[$class] = array(
        'position' => 'relative',
        'float' => 'left',
        'background-color' => 'transparent',
        'width' => $renderer->settings['items'][$id]['width'] . "px",
      );
    }

    // Do the same for right.
    $right_pixels = 0;

    foreach ($right as $id) {
      $class = "." . $renderer->base[$type] . "-$id";
      $css[$class] = array(
        'float' => 'left',
        'width' => $renderer->settings['items'][$id]['width'] . "px",
      );
    }

    $max = count($middle) - 1;

    if ($middle_total) {
      // Because we love IE so much, auto scale everything to 99%. This
      // means adding up the actual widths and then providing a multiplier
      // to each so that the total is 99%.
      $scale = $renderer->scale_base / $middle_total;
      foreach ($middle as $position => $id) {
        $class = "." . $renderer->base[$type] . "-$id";
        $css[$class] = array(
          'float' => 'left',
          'width' => number_format($renderer->settings['items'][$id]['width'] * $scale, 4, '.', '') . "%",
        );

        // Store this so we can use it later.
        // @todo: Store the scale, not the new width, so .js can adjust
        // bi-directionally.
        $renderer->scale[$id] = $scale;
      }
    }

    // If there is any total remaining, we need to offset the splitter
    // by this much too.
    if ($left_total) {
      // Add this even if it's 0 so we can handle removals.
      $css["$owner_id-inside"]['padding-left'] = '0px';
      if ($renderer->admin || count($middle)) {
        $css["$owner_id-middle"]['margin-left'] = $left_total . 'px';
        // IE hack
        $css["* html $owner_id-left"]['left'] = $left_total . "px";
        // Make this one very specific to the admin CSS so that preview
        // does not stomp it.
        $css[".panel-flexible-admin $owner_id-inside"]['padding-left'] = '0px';
      }
      else {
        $css["$owner_id-inside"]['margin-left'] = '-' . $left_total . 'px';
        $css["$owner_id-inside"]['padding-left'] = $left_total . 'px';
        // IE hack
        $css["* html $owner_id-inside"]['left'] = $left_total . "px";
      }
    }
    if ($right_total) {
      $css["$owner_id-middle"]['margin-right'] = $right_total . 'px';
    }
    $css["$owner_id-inside"]['padding-right'] = '0px';
  }

  // If the canvas has a fixed width set, and this is the canvas, fix the
  // width.
  if ($item_id == 'canvas') {
    $item = $renderer->settings['items'][$item_id];

    if (!empty($item['fixed_width']) && intval($item['fixed_width'])) {
      $css['.' . $renderer->base['canvas']]['width'] = intval($item['fixed_width']) . 'px';
    }
    else {
      $css['.' . $renderer->base['canvas']]['width'] = 'auto';
    }
  }

  // Go through each item and process children.
  foreach ($list as $id) {
    $item = $renderer->settings['items'][$id];
    if (empty($item['children'])) {
      continue;
    }

    if ($type == 'column') {
      // Columns can only contain rows.
      $child_type = 'row';
    }
    else {
      $child_type = isset($item['contains']) ? $item['contains'] : 'region';
    }
    $class = "." . $renderer->base[$type] . "-$id";
    flexible_layout_get_css_group($css, $renderer, $item['children'], $class, $child_type, $id);
  }
}

/**
 * AJAX responder to edit flexible settings for an item.
 *
 * $handler object
 *   The display renderer handler object.
 */
// function flexible_layout_ajax_flexible_edit_ajax_settings($handler, $id) {
function flexible_layout_ajax_flexible_edit_ajax_settings($form, $form_state) {
  $commands = array();
  $commands[] = ajax_command_close_modal_dialog();

  $handler = $form_state['handler'];
  $id = $form_state['id'];
  
  $renderer = flexible_layout_create_renderer(TRUE, array(), $settings, $handler->plugins['layout'], $handler);

  // If the item is a region, replace the title.
  $class = $renderer->base[$item['type']] . '-' . $id;
  if ($item['type'] == 'region') {
    $commands[] = ajax_command_replace(".$class h2.label",
      '<h2 class="label">' . check_plain($item['title']) . '</h2>');
  }

  // Rerender our links in case something changed.
  $commands[] = ajax_command_replace('.flexible-links-' . $id,
    flexible_layout_render_item_links($renderer, $id, $item));

  // If editing the canvas, reset the CSS width
  if ($id == 'canvas') {
    // update canvas CSS.
    $css = array(
      '.' . $renderer->item_class['column'] . '-inside' => array(
        'padding-left' => $renderer->column_separation,
        'padding-right' => $renderer->column_separation,
      ),
      '.' . $renderer->item_class['region'] . '-inside' => array(
        'padding-left' => $renderer->region_separation,
        'padding-right' => $renderer->region_separation,
      ),
      '.' . $renderer->item_class['row'] => array(
        'padding-bottom' => $renderer->row_separation,
      ),
    );
    if (!empty($item['fixed_width']) && intval($item['fixed_width'])) {
      $css['.' . $renderer->base['canvas']] = array('width' => intval($item['fixed_width']) . 'px');
    }
    else {
      $css['.' . $renderer->base['canvas']] = array('width' => 'auto');
    }
    foreach ($css as $selector => $data) {
      $commands[] = ajax_command_css($selector, $data);
    }
  }

  return array(
    '#type' => 'ajax',
    '#commands' => $commands,
  );
}

/**
 * Menu callback; Display a list of blocks to be added to a layout.
 */
function flexible_layout_edit_settings($form, &$form_state, Layout $layout, $renderer_name, $action, $id = NULL, $place = NULL) {
  // Todo: get from configuration
  $settings = $layout->settings['flexible'];
  $handler = layout_create_renderer($renderer_name, $layout);

  if (empty($settings['items'][$id])) {
    // ctools_modal_render(t('Error'), t('Invalid item id.'));
  }

  $item = &$settings['items'][$id];
  $siblings = array();

  if ($id != 'canvas') {
    $siblings = $settings['items'][$item['parent']]['children'];
  }


  switch ($item['type']) {
    case 'column':
      $title = t('Configure column');
      break;
    case 'row':
      if ($id == 'canvas') {
        $title = t('Configure canvas');
      }
      else {
        $title = t('Configure row');
      }
      break;
  }

  $form_state += array(
    'handler' => $handler,
    'item' => &$item,
    'id' => $id,
    'siblings' => $siblings,
    'settings' => &$settings,
    'ajax' => TRUE,
    'title' => $title,
    'op' => 'edit',
  );

  if ($item['type'] == 'region') {
    $form['title'] = array(
      '#title' => t('Region title'),
      '#type' => 'textfield',
      '#default_value' => $item['title'],
      '#required' => TRUE,
    );
  }

  if ($id == 'canvas') {
    $form['class'] = array(
      '#title' => t('Canvas class'),
      '#type' => 'textfield',
      '#default_value' => isset($item['class']) ? $item['class'] : '',
      '#description' => t('This class will the primary class for this layout. It will also be appended to all column, row and region_classes to ensure that layouts within layouts will not inherit CSS from each other. If left blank, the name of the layout or ID of the display will be used.'),
    );

    $form['column_class'] = array(
      '#title' => t('Column class'),
      '#type' => 'textfield',
      '#default_value' => isset($item['column_class']) ? $item['column_class'] : '',
      '#description' => t('This class will be applied to all columns of the layout. If left blank this will be flexible-layout-column.'),
    );

    $form['row_class'] = array(
      '#title' => t('Row class'),
      '#type' => 'textfield',
      '#default_value' => isset($item['row_class']) ? $item['row_class'] : '',
      '#description' => t('This class will be applied to all rows of the layout. If left blank this will be flexible-layout-row.'),
    );

    $form['region_class'] = array(
      '#title' => t('Region class'),
      '#type' => 'textfield',
      '#default_value' => isset($item['region_class']) ? $item['region_class'] : '',
      '#description' => t('This class will be applied to all regions of the layout. If left blank this will be flexible-layout-region.'),
    );

    $form['no_scale'] = array(
      '#type' => 'checkbox',
      '#title' => t('Scale fluid widths for IE6'),
      '#description' => t('IE6 does not do well with 100% widths. If checked, width will be scaled to 99% to compensate.'),
      '#default_value' => empty($item['no_scale']),
    );

    $form['fixed_width'] = array(
      '#type' => 'textfield',
      '#title' => t('Fixed width'),
      '#description' => t('If a value is entered, the layout canvas will be fixed to the given pixel width.'),
      '#default_value' => isset($item['fixed_width']) ? $item['fixed_width'] : '',
    );

    $form['column_separation'] = array(
      '#type' => 'textfield',
      '#title' => t('Column separation'),
      '#description' => t('How much padding to put on columns that are that are next to other columns. Note that this is put on both columns so the real amount is doubled.'),
      '#default_value' => isset($item['column_separation']) ? $item['column_separation'] : '0.5em',
    );

    $form['region_separation'] = array(
      '#type' => 'textfield',
      '#title' => t('Region separation'),
      '#description' => t('How much padding to put on regions that are that are next to other regions. Note that this is put on both regions so the real amount is doubled.'),
      '#default_value' => isset($item['region_separation']) ? $item['region_separation'] : '0.5em',
    );

    $form['row_separation'] = array(
      '#type' => 'textfield',
      '#title' => t('Row separation'),
      '#description' => t('How much padding to put on beneath rows to separate them from each other. Because this is placed only on the bottom, not hte top, this is NOT doubled like column/region separation.'),
      '#default_value' => isset($item['row_separation']) ? $item['row_separation'] : '0.5em',
    );
  }
  else {
    $form['class'] = array(
      '#title' => t('CSS class'),
      '#type' => 'textfield',
      '#default_value' => isset($item['class']) ? $item['class'] : '',
      '#description' => t('Enter a CSS class that will be used. This can be used to apply automatic styling from your theme, for example.'),
    );

    if ($item['type'] != 'row') {
      // Test to see if there are fluid items to the left or the right. If there
      // are fluid items on both sides, this item cannot be set to fixed.
      $left = $right = FALSE;
      $current = 'left';
      foreach ($siblings as $sibling) {
        if ($sibling == $id) {
          $current = 'right';
        }
        else if ($settings['items'][$sibling]['width_type'] == '%') {
          $$current = TRUE; // Indirection.
        }
      }

      $form['width_type'] = array(
        '#type' => 'select',
        '#title' => t('Width'),
        '#default_value' => $item['width_type'],
        '#options' => array(
          '%' => t('Fluid'),
          'px' => t('Fixed'),
        ),
        '#disabled' => TRUE,
      );
    }
    else {
      $form['contains'] = array(
        '#type' => 'select',
        '#title' => t('Contains'),
        '#default_value' => $item['contains'],
        '#options' => array(
          'region' => t('Regions'),
          'column' => t('Columns'),
        ),
      );

      if (!empty($item['children'])) {
        $form['contains']['#disabled'] = TRUE;
        $form['contains']['#value'] = $item['contains'];
        $form['contains']['#description'] = t('You must remove contained items to change the row container type.');
      }
    }
  }

  $form['#tree'] = TRUE;
  $form['hide_empty'] = array(
    '#title' => t('Hide element if empty'),
    '#type' => 'checkbox',
    '#default_value' => !empty($item['hide_empty']) ? 1 : 0,
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configurationnnn'),
    '#attributes' => array('class' => array('layout-title-button')),
    '#submit' => array(
      'flexible_layout_config_item_form_submit',
    ),
    '#ajax' => array(
      'callback' => 'flexible_layout_ajax_flexible_edit_ajax_settings',
    ),
  );

  return $form;
}


/**
 * Submit handler for editing a flexible item.
 */
function flexible_layout_config_item_form_submit(&$form, &$form_state) {
  $item = &$form_state['item'];
  if ($item['type'] == 'region') {
    $item['title'] = $form_state['values']['title'];
  }

  $item['class'] = $form_state['values']['class'];

  if ($form_state['id'] == 'canvas') {
    $item['column_class'] = $form_state['values']['column_class'];
    $item['row_class'] = $form_state['values']['row_class'];
    $item['region_class'] = $form_state['values']['region_class'];
    // Reverse this as the checkbox is backward from how we actually store
    // it to make it simpler to default to scaling.
    $item['no_scale'] = !$form_state['values']['no_scale'];
    $item['fixed_width'] = $form_state['values']['fixed_width'];
    $item['column_separation'] = $form_state['values']['column_separation'];
    $item['region_separation'] = $form_state['values']['region_separation'];
    $item['row_separation'] = $form_state['values']['row_separation'];
  }
  else if ($item['type'] != 'row') {
    $item['width_type'] = $form_state['values']['width_type'];
  }
  else {
    $item['contains'] = $form_state['values']['contains'];
  }
  $item['hide_empty'] = $form_state['values']['hide_empty'];

}

/**
 * AJAX responder to add a new row, column or region to a flexible layout.
 */
function flexible_layout_ajax_flexible_edit_add($form, $form_state) {
  $commands = array();
  $commands[] = ajax_command_close_modal_dialog();

  $handler = $form_state['handler'];
  $id = $form_state['id'];
  $location = isset($form_state['location']) ? $form_state['location'] : 'left';
  $item = $form_state['item'];
  $parent = $form_state['parent'];
  $settings = $form_state['settings'];
  $layout = $form_state['layout'];

  // Create a renderer object so we can render our new stuff.
  $renderer = flexible_layout_create_renderer(TRUE, array(), $settings, $layout, $handler);

  $content = '';
  if ($item['type'] == 'region') {
    $handler->layout_info['regions'][$form_state['key']] = $item['title'];

    $content = $handler->renderRegion($form_state['key'], array());

    // Manually add the hidden field that our region uses to store pane info.
    $content .= '<input type="hidden" name="panel[pane][' .
      $form_state['key'] . ']" value="" />';

  }
  else {
    // We need to make sure the left/middle/right divs exist inside this
    // so that more stuff can be added inside it as needed.
    foreach (array('left', 'middle', 'right') as $position) {
      if (!empty($content) || $renderer->admin) {
        $content .= '<div class="' . $renderer->base[$item['type']] . '-' . $form_state['key'] . '-' . $position . '"></div>';
      }
    }

  }

  // render the item
  $parent_class =  $renderer->base[$parent['type']] . '-' . $id;

  $item_output = flexible_layout_render_item($renderer, $item, $content, $form_state['key'], 0, 0, $item['type'] == 'row');

  // Get all the CSS necessary for the entire row (as width adjustments may
  // have cascaded).
  $css = array();
  flexible_layout_get_css_group($css, $renderer, $parent['children'], '.' . $parent_class, $item['type'], $id);

  $position = isset($renderer->positions[$form_state['key']]) ? $renderer->positions[$form_state['key']] : 'middle';
  // If there's a nearby item, add the splitter and rewrite the width
  // of the nearby item as it probably got adjusted.
  // The blocks of code in this else look very similar but are not actually
  // duplicated because the order changes based on left or right.
  switch ($position) {
    case 'left':
      if ($location == 'left') {
        $item_output .= flexible_layout_render_splitter($renderer, $form_state['key'], $form_state['sibling']);
        $commands[] = ajax_command_prepend('#layout-edit-main .' . $parent_class . '-left', $item_output);
      }
      else if ($location == 'right') {
        // If we are adding to the right side of the left box, there is
        // a splitter that we have to remove; then we add our box normally,
        // and then add a new splitter for just our guy.
        $commands[] = ajax_command_remove('flexible-layout-splitter-for-' . $renderer->base[$item['type']] . '-' . $form_state['key']);
        $item_output = flexible_layout_render_splitter($renderer, $form_state['sibling'], $form_state['key']) .  $item_output;
        $item_output .= flexible_layout_render_splitter($renderer, $form_state['key'], NULL);
        $commands[] = ajax_command_append('#layout-edit-main .' . $parent_class . '-left', $item_output);
      }
      break;
    case 'right':
      if (!empty($form_state['sibling'])) {
        $item_output = flexible_layout_render_splitter($renderer, $form_state['sibling'], $form_state['key']) .  $item_output;
      }
      $commands[] = ajax_command_append('#layout-edit-main .' . $parent_class . '-right', $item_output);
      break;
    case 'middle':
      if ($location == 'left') {
        if (!empty($form_state['sibling'])) {
          $item_output .= flexible_layout_render_splitter($renderer, $form_state['key'], $form_state['sibling']);
        }
        $commands[] = ajax_command_prepend('#layout-edit-main .' . $parent_class . '-middle', $item_output);
      }
      else {
        if (!empty($form_state['sibling'])) {
          $item_output = flexible_layout_render_splitter($renderer, $form_state['sibling'], $form_state['key']) .  $item_output;
        }
        $commands[] = ajax_command_append('#layout-edit-main .' . $parent_class . '-middle', $item_output);
      }
      break;

  }

  // Send our fix height command.
  $commands[] = array('command' => 'flexible_fix_height');

  if (!empty($form_state['sibling'])) {
    $sibling_width = '#layout-edit-main .' . $renderer->base[$item['type']] . '-' . $form_state['sibling'] . '-width';
    $commands[] = array(
      'command' => 'flexible_set_width',
      'selector' => $sibling_width,
      'width' => $settings['items'][$form_state['sibling']]['width'],
    );
  }
  foreach ($css as $selector => $data) {
    $commands[] = ajax_command_css($selector, $data);
  }

  // Rerender our parent item links:
  $commands[] = ajax_command_replace('.flexible-links-' . $id, flexible_layout_render_item_links($renderer, $id, $parent));

  $commands[] = array(
    'command' => 'flexible_fix_firstlast',
    'selector' => '.' . $parent_class . '-inside',
    'base' => 'flexible-layout-' . $item['type'],
  );

  return array(
    '#type' => 'ajax',
    '#commands' => $commands,
  );
}
/**
 * Form to add a row, column or region to a flexible layout.
 * @param <type> $form_state
 * @return <type>
 */
function flexible_layout_add_item_form($form, &$form_state, Layout $layout, $renderer_name, $action, $id = NULL, $location = 'left') {
  // Todo: get from configuration
  $settings = $layout->settings['flexible'];
  $handler = layout_create_renderer($renderer_name, $layout);

  if (!isset($form_state['layout'])) {
    $form_state['layout'] = $layout;
  }

  if (empty($settings['items'][$id])) {
    // ctools_modal_render(t('Error'), t('Invalid item id.'));
  }

  $parent = &$settings['items'][$id];

  switch ($parent['type']) {
    case 'column':
      $title = t('Add row');
      // Create the new item with defaults.
      $item = array(
        'type' => 'row',
        'contains' => 'region',
        'children' => array(),
        'parent' => $id,
      );
      break;
    case 'row':
      switch ($parent['contains']) {
        case 'region':
          $title = $location == 'left' ? t('Add region to left') : t('Add region to right');
          $item = array(
            'type' => 'region',
            'title' => '',
            'width' => 100,
            'width_type' => '%',
            'parent' => $id,
          );
          break;
        case 'column':
          $title = $location == 'left' ? t('Add column to left') : t('Add column to right');
          $item = array(
            'type' => 'column',
            'width' => 100,
            'width_type' => '%',
            'parent' => $id,
            'children' => array(),
          );
          break;
      }
      // Create the new item with defaults.
      break;
    case 'region':
      // Cannot add items to regions.
      break;
  }

  $form_state += array(
    'handler' => &$handler,
    'parent' => &$parent,
    'item' => &$item,
    'id' => $id,
    'settings' => &$settings,
    'ajax' => TRUE,
    'title' => $title,
    'location' => $location,
  );

  if ($item['type'] == 'region') {
    $form['title'] = array(
      '#title' => t('Region title'),
      '#type' => 'textfield',
      '#default_value' => $item['title'],
      '#required' => TRUE,
    );
  }

  $form['class'] = array(
    '#title' => t('CSS Class'),
    '#type' => 'textfield',
    '#default_value' => isset($item['class']) ? $item['class'] : '',
    '#description' => t('Enter a CSS class that will be used. This can be used to apply automatic styling from your theme, for example.'),
  );

  if ($item['type'] != 'row') {
    // If there is a 'fixed' type on the side we're adding to, then this
    // must also be fixed. Otherwise it can be either and should default to
    // fluid.
    $restrict = FALSE;

    if (!empty($parent['children'])) {
      if ($location == 'left') {
        $sibling = reset($parent['children']);
      }
      else {
        $sibling = end($parent['children']);
      }
      if ($settings['items'][$sibling]['width_type'] == 'px') {
        $restrict = TRUE;
        $item['width_type'] = 'px';
      }
    }

    $form['width_type'] = array(
      '#type' => 'select',
      '#title' => t('Width'),
      '#default_value' => $item['width_type'],
      '#options' => array(
        '%' => t('Fluid'),
        'px' => t('Fixed'),
      ),
      '#disabled' => $restrict,
    );
    if ($restrict) {
      // This forces the value because disabled items don't always send
      // their data back.
      $form['width_type']['#value'] = $item['width_type'];
      $form['width_type']['#description'] = t('Items cannot be set to fluid if there are fixed items already on that side.');
    }
  }
  else {
    $form['contains'] = array(
      '#type' => 'select',
      '#title' => t('Contains'),
      '#default_value' => $item['contains'],
      '#options' => array(
        'region' => t('Regions'),
        'column' => t('Columns'),
      ),
    );
  }

  $form['hide_empty'] = array(
    '#title' => t('Hide element if empty'),
    '#type' => 'checkbox',
    '#default_value' => 0,
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configurationnnn'),
    '#attributes' => array('class' => array('layout-title-button')),
    '#submit' => array(
      'flexible_layout_add_item_form_submit',
    ),
    '#ajax' => array(
      'callback' => 'flexible_layout_ajax_flexible_edit_add',
    ),
  );

  return $form;
}

/**
 * Submit handler for editing a flexible item.
 */
function flexible_layout_add_item_form_submit(&$form, &$form_state) {
  $item = &$form_state['item'];
  $parent = &$form_state['parent'];
  $location = &$form_state['location'];
  $settings = &$form_state['settings'];
  $layout = &$form_state['layout'];

  $item['class'] = $form_state['values']['class'];

  if ($item['type'] == 'region') {
    $item['title'] = $form_state['values']['title'];
  }

  if ($item['type'] != 'row') {
    $item['width_type'] = $form_state['values']['width_type'];
  }
  else {
    $item['contains'] = $form_state['values']['contains'];
  }

  $item['hide_empty'] = $form_state['values']['hide_empty'];

  if ($item['type'] == 'region') {
    // derive the region key from the title
    $key = preg_replace("/[^a-z0-9]/", '_', backdrop_strtolower($item['title']));
    while (isset($settings['items'][$key])) {
      $key .= '_';
    }
    $form_state['key'] = $key;
  }
  else {
    $form_state['key'] = $key = max(array_keys($settings['items'])) + 1;
  }

  $form_state['sibling'] = NULL;
  if ($item['type'] != 'row' && !empty($parent['children'])) {
    // Figure out what the width should be and adjust our sibling if
    // necessary.
    if ($location == 'left') {
      $form_state['sibling'] = reset($parent['children']);
    }
    else {
      $form_state['sibling'] = end($parent['children']);

    }

    // If there is no sibling, or the sibling is of a different type,
    // the default 100 will work for either fixed or fluid.
    if ($form_state['sibling'] && $settings['items'][$form_state['sibling']]['width_type'] == $item['width_type']) {
      // steal half of the sibling's space.
      $width = $settings['items'][$form_state['sibling']]['width'] / 2;
      $settings['items'][$form_state['sibling']]['width'] = $width;
      $item['width'] = $width;
    }
  }

  // Place the item.
  $settings['items'][$key] = $item;
  if ($location == 'left') {
    array_unshift($parent['children'], $key);
  }
  else {
    $parent['children'][] = $key;
  }
  // $settings['items'][$form_state['id']] = $parent;
  $layout->settings['flexible'] = $settings;
  layout_set_layout_tempstore($layout);
  $form_state['redirect'] = 'admin/structure/layouts/manage/' . $layout->name;
}

/**
 * AJAX responder to remove an existing row, column or region from a flexible
 * layout.
 */
function flexible_layout_ajax_flexible_edit_remove($layout, $renderer_name, $id) {
  $settings = $layout->settings['flexible'];
  $handler = layout_create_renderer($renderer_name, $layout);
  $commands = array();

  if (empty($settings['items'][$id])) {
    ajax_render_error(t('Invalid item id.'));
  }

  $item = &$settings['items'][$id];
  // Create a renderer object so we can render our new stuff.
  $renderer = flexible_layout_create_renderer(TRUE, array(), $settings, $layout, $handler);


  $siblings = &$settings['items'][$item['parent']]['children'];
  $parent_class = '.'  . $renderer->base[$settings['items'][$item['parent']]['type']] . '-' . $item['parent'];

  // Find the offset of our array. This will also be the key because
  // this is a simple array.
  $offset = array_search($id, $siblings);

  // Only bother with this stuff if our item is fluid, since fixed is
  // as fixed does.
  if ($item['type'] != 'row') {
    if (isset($siblings[$offset + 1])) {
      $next = $siblings[$offset + 1];
    }
    if (isset($siblings[$offset - 1])) {
      $prev = $siblings[$offset - 1];
    }

    if ($item['width_type'] == '%') {
      // First, try next.
      if (isset($next) && $settings['items'][$next]['width_type'] == '%') {
        $settings['items'][$next]['width'] += $item['width'];
      }
      // If that failed, try the previous one.
      else if (isset($prev) && $settings['items'][$prev]['width_type'] == '%') {
        $settings['items'][$prev]['width'] += $item['width'];
      }
    }
    // Not sure what happens if they both failed. Maybe nothing.
  }

  // Remove the item.
  array_splice($siblings, $offset, 1);

  unset($settings['items'][$id]);

  // Save our new state.
  $layout->settings['flexible'] = $settings;
  layout_set_layout_tempstore($layout);

  $class = $renderer->base[$item['type']] . '-' . $id;
  $commands = array();

  $commands[] = ajax_command_remove('#layout-edit-main .' . $class);

  // Regenerate the CSS for siblings.
  if (!empty($siblings)) {
    // Get all the CSS necessary for the entire row (as width adjustments may
    // have cascaded).
    $css = array();
    flexible_layout_get_css_group($css, $renderer, $siblings, $parent_class, $item['type'], $item['parent']);
    foreach ($css as $selector => $data) {
      $commands[] = ajax_command_css($selector, $data);
    }
  }

  // There are potentially two splitters linked to this item to be removed.
  if (!empty($prev)) {
    $commands[] = ajax_command_remove('.flexible-splitter-for-' . $renderer->base[$item['type']] . '-' . $prev);
  }

  // Try to remove the 'next' one even if there isn't a $next.
  $commands[] = ajax_command_remove('.flexible-splitter-for-' . $renderer->base[$item['type']] . '-' . $id);

  if (!empty($prev) && !empty($next)) {
    // Add a new splitter that links $prev and $next:
    $splitter = flexible_layout_render_splitter($renderer, $prev, $next);
    $prev_class = '#layout-edit-main .' . $renderer->base[$item['type']] . '-' . $prev;
    $commands[] = ajax_command_after($prev_class, $splitter);
    // Send our fix height command.
    $commands[] = array('command' => 'flexible_fix_height');
  }
  // Rerender our parent item links:
  $commands[] = ajax_command_replace('.flexible-links-' . $item['parent'],
    flexible_layout_render_item_links($renderer, $item['parent'], $settings['items'][$item['parent']]));

  $commands[] = array(
    'command' => 'flexible_fix_firstlast',
    'selector' => $parent_class . '-inside',
    'base' => 'flexible-layout-' . $item['type'],
  );

  return array(
    '#type' => 'ajax',
    '#commands' => $commands,
  );
}

/**
 * AJAX responder to store resize information when the user adjusts the
 * splitter.
 */
function flexible_layout_ajax_flexible_edit_resize($handler) {
  ctools_include('ajax');
  $settings = &$handler->display->layout_settings;
  flexible_layout_convert_settings($settings, $handler->plugins['layout']);

  $settings['items'][$_POST['left']]['width'] = $_POST['left_width'];
  if (!empty($_POST['right']) && $_POST['right'] != $_POST['left']) {
    $settings['items'][$_POST['right']]['width'] = $_POST['right_width'];
  }

  // Save our new state.
  // panels_edit_cache_set($handler->cache);

  $handler->commands = array('ok');
}
