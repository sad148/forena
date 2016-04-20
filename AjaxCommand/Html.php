<?php
/**
 * Created by PhpStorm.
 * User: metzlerd
 * Date: 4/17/16
 * Time: 4:25 PM
 */

namespace Drupal\forena\FrxPlugin\AjaxCommand;


/**
 * Html command
 * 
 * @FrxAjaxCommand(
 *   id = "html"
 * )
 */
class Html extends AjaxCommandBase {

  /**
   * JQuery Html Command
   * 
   * Settings: 
   *   - selector: JQuery selector to use for Html. 
   *   - text: HTML to use to replace
   * 
   * @param array $settings
   *   Settings for command
   * @return array
   */
  public function commandFromSettings(array $settings) {
    $text = $this->getSetting($settings, 'text'); 
    $selector = $this->getSetting($settings, 'selector'); 
    return ajax_command_html($selector, $text, $settings); 
  }
}