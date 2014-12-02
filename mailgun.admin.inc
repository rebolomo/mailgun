<?php
/**
 * @file
 * Administrative forms for mailgun module.
 */


/**
 * Administrative settings.
 *
 * @return
 *   An array containing form items to place on the module settings page.
 */
function mailgun_admin_settings($form, &$form_state) {
  $key = variable_get('mailgun_api_key');
  $form['mailgun_api_key'] = array(
    '#title' => t('Mailgun API Key'),
    '#type' => 'textfield',
    '#description' => t('Create or grab your API key from the !link.',
      array('!link' => l('mailgun settings', 'http://www.mailgun.com/'))),
    '#default_value' => $key,
  );

  $domain = variable_get('mailgun_domain');
  $form['mailgun_domain'] = array(
    '#title' => t('Mailgun Domain'),
    '#type' => 'textfield',
    '#description' => t('Domain for sending mail'),
    '#default_value' => $domain,
  );

  if ($key && $domain) {
    $form['mailgun_status'] = array(
      '#type' => 'radios',
      '#title' => t('mailgun Mail interface status'),
      '#default_value' => variable_get('mailgun_status', MAIL_GUN_STATUS_OFF),
      '#options' => array(
        MAIL_GUN_STATUS_ON => t('On'),
        MAIL_GUN_STATUS_TEST => t('Test'),
        MAIL_GUN_STATUS_OFF => t('Off')
      ),
      '#description' => t('Setting to on routes all site emails through the mailgun
        API. Test mode implements an alternate mail interface,
        TestingMailChimpMailGunMailSystem, that does not send any emails, it just
        displays a message and logs the event.'),
    );

    $form['email_options'] = array(
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#title' => t('Email options'),
    );
    $form['email_options']['mailgun_from'] = array(
      '#title' => t('From address'),
      '#type' => 'textfield',
      '#description' => t('The sender email address. If this address has not been verified, messages will be queued and not sent until it is.'),
      '#default_value' => variable_get('mailgun_from', variable_get('site_mail'))
    );
    $form['email_options']['mailgun_from_name'] = array(
      '#type' => 'textfield',
      '#title' => t('From name'),
      '#default_value' => variable_get('mailgun_from_name', ''),
      '#description' => t('Optionally enter a from name to be used.'),
    );

    $formats = filter_formats();
    $options = array('' => t('-- Select --'));
    foreach ($formats AS $v => $format) {
      $options[$v] = $format->name;
    }
    $form['email_options']['mailgun_filter_format'] = array(
      '#type' => 'select',
      '#title' => t('Input format'),
      '#description' => t('If selected, the input format to apply to the message body before sending to the mailgun API.'),
      '#options' => $options,
      '#default_value' => array(variable_get('mailgun_filter_format', 'full_html')),
    );

    $form['send_options'] = array(
      '#title' => t('Send options'),
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
    );
    $form['send_options']['mailgun_track_opens'] = array(
      '#title' => t('Track opens'),
      '#type' => 'checkbox',
      '#description' => t('Whether or not to turn on open tracking for messages.'),
      '#default_value' => variable_get('mailgun_track_opens', TRUE)
    );
    $form['send_options']['mailgun_track_clicks'] = array(
      '#title' => t('Track clicks'),
      '#type' => 'checkbox',
      '#description' => t('Whether or not to turn on click tracking for messages.'),
      '#default_value' => variable_get('mailgun_track_clicks', TRUE)
    );
    $form['send_options']['mailgun_url_strip_qs'] = array(
      '#title' => t('Strip query string'),
      '#type' => 'checkbox',
      '#description' => t('Whether or not to strip the query string from URLs when aggregating tracked URL data.'),
      '#default_value' => variable_get('mailgun_url_strip_qs', FALSE)
    );
    $form['send_options']['mailgun_mail_key_blacklist'] = array(
      '#title' => t('Content logging blacklist'),
      '#type' => 'textarea',
      '#description' => t('Comma delimited list of Drupal mail keys to exclude content logging for. CAUTION: Removing the default password reset key may expose a security risk.'),
      '#default_value' => mailgun_mail_key_blacklist()
    );

    $form['analytics'] = array(
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#title' => t('Google analytics'),
    );
    $form['analytics']['mailgun_analytics_domains'] = array(
      '#title' => t('Google analytics domains'),
      '#type' => 'textfield',
      '#description' => t('One or more domains for which any matching URLs will automatically have Google Analytics parameters appended to their query string. Separate each domain with a comma.'),
      '#default_value' => variable_get('mailgun_analytics_domains', '')
    );
    $form['analytics']['mailgun_analytics_campaign'] = array(
      '#title' => t('Google analytics campaign'),
      '#type' => 'textfield',
      '#description' => t('The value to set for the utm_campaign tracking parameter. If this isn\'t provided the messages from address will be used instead.'),
      '#default_value' => variable_get('mailgun_analytics_campaign', '')
    );

    $form['#submit'][] = 'mailgun_admin_settings_submit';
  }

  return system_settings_form($form);
}

/**
 * Submission for the administrative settings form.
 *
 * @param form
 *   An associative array containing the structure of the form.
 * @param form_state
 *   A keyed array containing the current state of the form.
 */
function mailgun_admin_settings_submit($form, &$form_state) {
  $values = $form_state['values'];

  switch ($form_state['values']['mailgun_status']) {
    case MAIL_GUN_STATUS_ON:
			//	REBOL edit, fix the mailgun but, it kills my own mail system settings.
			$mail_system = variable_get('mail_system');
			$mail_system['default-system'] = 'MailGunMailSystem';
			variable_set('mail_system', $mail_system);
      //variable_set('mail_system', array('default-system' => 'MailGunMailSystem'));
      drupal_set_message(t('mailgun will be used to deliver all site emails.'));
      watchdog('mailgun', 'mailgun has been enabled.');
      break;
    case MAIL_GUN_STATUS_TEST:
			$mail_system = variable_get('mail_system');
			$mail_system['default-system'] = 'TestingMailGunMailSystem';
			variable_set('mail_system', $mail_system);
      //variable_set('mail_system', array('default-system' => 'TestingMailGunMailSystem'));
      drupal_set_message(t('mailgun will be used in test mode. Emails will not actually be sent, just logged.'));
      watchdog('mailgun', 'mailgun has been placed in test mode.');
      break;
    case MAIL_GUN_STATUS_OFF:
			$mail_system = variable_get('mail_system');
			$mail_system['default-system'] = 'DefaultMailSystem';
			variable_set('mail_system', $mail_system);
      //variable_set('mail_system', array('default-system' => 'DefaultMailSystem'));
      drupal_set_message(t('The default mail system will be used to deliver all site emails.'));
      watchdog('mailgun', 'mailgun has been disabled.');
      break;
  }
}

/**
 * Return a form for sending a test email.
 *
 * @param string $form
 * @param string $form_state
 *
 * @return array $form
 */
function mailgun_test_form($form, &$form_state) {
  drupal_set_title(t('Send test email'));

  $form['mailgun_test_address'] = array(
    '#type' => 'textfield',
    '#title' => t('Email address to send a test email to'),
    '#default_value' => 'rebol@126.com',//variable_get('site_mail', ''),
    '#description' => t('Type in an address to have a test email sent there.'),
    '#required' => TRUE
  );
  $form['mailgun_test_body'] = array(
    '#type' => 'textarea',
    '#title' => t('Test body contents'),
    '#default_value' => t('If you receive this message it means your site is capable of using mailgun to send email. This <a href="http://drupal.org">link</a> is here to test click tracking.'),
  );
  $form['include_attachment'] = array(
    '#title' => t('Include attachment'),
    '#type' => 'checkbox',
    '#description' => t('If checked, the Drupal icon will be included as an attachment with the test email.'),
    '#default_value' => true
  );

  $form['test_submit'] = array(
    '#type' => 'submit',
    '#value' => t('Send test email'),
  );
  $form['test_cancel'] = array(
    '#type' => 'link',
    '#href' => 'admin/config/services/mailgun',
    '#title' => t('Cancel'),
  );

  return $form;
}

/**
 * Submit handler for mailgun_test_form(), sends the test email.
 *
 * @param string $form
 * @param string $form_state
 *
 * @return void
 */
function mailgun_test_form_submit($form, &$form_state) {
  // If an address was given, send a test email message.
  $test_address = $form_state['values']['mailgun_test_address'];
  global $language;
  $params['subject'] = t('Drupal mailgun test email');
  $params['body'] = $form_state['values']['mailgun_test_body'];
  $params['include_attachment'] = $form_state['values']['include_attachment'];
	$from = variable_get('mailgun_from', '');
	//dsm($params);
	//dsm($from);
  $result = drupal_mail('mailgun', 'mailgun-test', $test_address, $language, $params, $from);
	//dsm($result);
  if (isset($result['result']) && $result['result'] === true) {
    drupal_set_message(t('A test email has been sent to @email.', array('@email' => $test_address)));
  }
}
