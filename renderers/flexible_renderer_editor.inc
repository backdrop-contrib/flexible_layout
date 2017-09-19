<?php

/**
 * @file
 * Class file to control the main Layout editor.
 */

class FlexibleLayoutRendererEditor extends LayoutRendererEditor {
  /**
   * {@inheritdoc}
   */
  function __construct(Layout $layout, array $renderer_plugin) {
    parent::__construct($layout, $renderer_plugin);

    // Set the page title region if not already.
    if (!isset($this->layout_info['regions']['title'])) {
      $this->layout_info['regions']['title'] = t('Page title');
    }
    $regions = flexible_get_regions($layout->settings['flexible']);

    foreach ($regions as $key => $region) {
      $this->layout_info['regions'][$key] = $region;
    }
    $this->admin = TRUE;
  }

  /**
   * Perform display/layout-level render operations.
   *
   * This method triggers all the inner block/region rendering processes, passes
   * that to the layout plugin's theme callback, and returns the rendered HTML.
   *
   * If display-level caching is enabled and that cache is warm, this method
   * will not be called.
   *
   * @return string
   *   The HTML string representing the entire rendered, themed layout.
   */
  function renderLayout() {
    if (empty($this->prep_run)) {
      $this->prepare();
    }
    $this->renderBlocks();

    // Return 404 or 403 if set by the main content block.
    if ($this->menu_response) {
      return $this->menu_response;
    }

    $this->renderRegions();

    module_load_include('inc', 'layout', 'layout.theme');

    $this->rendered['layout'] = theme('build_flexible_layout', array('content' => $this->rendered['regions'], 'settings' => $this->layout->settings, 'layout' => $this->layout, 'renderer' => $this));
    return $this->prefix . $this->rendered['layout'] . $this->suffix;
  }

  /**
   * Render the links to display when editing a region.
   */
  protected function getRegionLinks($region_id) {
    $links = array();
    if ($region_id == 'title') {
      $links['title'] = array(
        'title' => t('Configure'),
        'href' => $this->getUrl('edit-title', $region_id),
        'attributes' => array(
          'class' => array('use-ajax'),
          'data-dialog' => TRUE,
          'data-dialog-options' => json_encode(array('dialogClass' => 'layout-dialog')),
        ),
      );
    }
    else {
      $links['add'] = array(
        'title' => t('Add block'),
        'href' => $this->getUrl('add-flexible-block', $region_id),
        'attributes' => array(
          'class' => array('use-ajax'),
          'data-dialog' => TRUE,
          'data-dialog-options' => json_encode(array('dialogClass' => 'layout-dialog')),
        ),
      );
      $links['configure'] = array(
        'title' => t('Configure region'),
        'href' => $this->getUrl('configure-region', $region_id),
        'attributes' => array(
          'class' => array('use-ajax'),
          'data-dialog' => TRUE,
          'data-dialog-options' => json_encode(array('dialogClass' => 'layout-dialog')),
        ),
      );
    }

    $dropbutton = array(
      '#type' => 'dropbutton',
      '#links' => $links,
    );

    return backdrop_render($dropbutton);
  }

}
