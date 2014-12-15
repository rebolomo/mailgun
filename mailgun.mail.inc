<?php

/**
 * @file
 * Implements mailgun as a Drupal MailSystemInterface
 */

/**
 * Modify the drupal mail system to use mailgun when sending emails.
 */
class MailGunMailSystem implements MailSystemInterface {

  /**
   * Concatenate and wrap the email body for either
   * plain-text or HTML emails.
   *
   * @param $message
   *   A message array, as described in hook_mail_alter().
   *
   * @return
   *   The formatted $message.
   */
  public function format(array $message) {
    // Join the body array into one string.
    if (is_array($message['body'])) {
      $message['body'] = implode("\n\n", $message['body']);
    }
    return $message;
  }

  /**
   * Send the email message.
   *
   * @see drupal_mail()
   *
   * @param $message
   *   A message array, as described in hook_mail_alter().
   *
   * @return
   *   TRUE if the mail was successfully accepted, otherwise FALSE.
   */
  public function mail(array $message) {
    $from = $message['from'];
    if (variable_get('mailgun_verify_from')) {
      if ($from != variable_get('mailgun_from', '')) {
        watchdog('mailgun', 'Email sent from unverified address: @from',
          array('@from' => $from));
        return FALSE;
      }
    }

    // send the email passing the message id as the tag for use in reporting
    $mailer = mailgun_get_api_object();
		//dsm($mailer);
    // apply input format to body
    $html = $message['body'];
    $format = variable_get('mailgun_filter_format', '');
    if (!empty($format)) {
      $html = check_markup($message['body'], $format);
    }

    //$to = mailgun_get_to($message['to']);
		$to = $message['to'];

    // determine if content should be available for this message
    $blacklisted_keys = explode(',', mailgun_mail_key_blacklist());
    $view_content = TRUE;
    foreach ($blacklisted_keys as $key) {
      if  ($message['id'] == drupal_strtolower(trim($key))) {
        $view_content = FALSE;
        break;
      }
    }

    $mailgun_message = array(
      'html' => $html,
      'text' => drupal_html_to_text($message['body']),
      'subject' => $message['subject'],
      /*'from_name' => variable_get('mailgun_from_name', ''),*/
      'from' => $from,
      'to' => $to,
      // optional extra headers to add to the message (currently only Reply-To and X-* headers are allowed)
      /*'headers' => $message['headers'],*/
			'o:tracking' => true,
      /*'track_opens'*/'o:tracking-opens' => variable_get('mailgun_track_opens', TRUE),
      /*'track_clicks'*/'o:tracking-clicks' => variable_get('mailgun_track_clicks', TRUE),
      // we're handling this with drupal_html_to_text().
      //'auto_text' => FALSE,
      //'url_strip_qs' => variable_get('mailgun_url_strip_qs', FALSE),
      /*'bcc_address'*/
      /*'tags'*/'o:tag' => array($message['id']),
      //'google_analytics_domains' => (variable_get('mailgun_analytics_domains', NULL)) ? explode(',', variable_get('mailgun_analytics_domains')) : array(),
      //'google_analytics_campaign' => variable_get('mailgun_analytics_campaign', ''),
      /*'attachments'*/
      //'view_content_link' => $view_content
    );

		if (isset($message['attachments']) && !empty($message['attachments'])) {
			$i = 1;
      foreach ($message['attachments'] as $attachment) {
				if ( is_file( $attachment ) ) {
					$mailgun_message["attachment[{$i}]"] = "@{$attachment}";
					$i++;
				}
			}
		}

		if(isset($message['bcc_email'])) {
			$mailgun_message['bcc'] = $message['bcc_email'];
		}

    drupal_alter('mailgun_mail', $mailgun_message, $message);

    try {
      $result = $mailer->messages_send($mailgun_message);
      // @todo: look for rejected messages and log
      return TRUE;
    }
    catch (MailGun_Exception $e) {
      watchdog('mailgun', 'Error sending email from %from to %to. @code: @message',
        array(
          '%from' => $from,
          '%to' => $message['to'],
          '@code' => $e->getCode(),
          '@message' => $e->getMessage(),
        ),
        WATCHDOG_ERROR
      );

      return FALSE;
    }
  }

}

/**
 * A mail sending implementation that captures sent messages to watchdog.
 *
 * This class is for running tests or for development.
 */
class TestingMailGunMailSystem extends MailGunMailSystem implements MailSystemInterface {

  /**
   * Accept an e-mail message and store it in a variable.
   *
   * @param $message
   *   An e-mail message.
   */
  public function mail(array $message) {
    if (!$from = variable_get('mailgun_from', '')) {
      drupal_set_message(t('mailgun can\'t send email. Please !link.',
        array(
          '!link' => l('add a verified from address',
            'admin/config/services/mailchimp/sts'
          )
        )
      ), 'error');
      return FALSE;
    }
    //$to = mailgun_get_to($message['to']);
		$to = $message['to'];
    watchdog('mailgun', 'mailgun test email sent from %from to %to. Message: <pre>@message</pre>',
      array(
        '%from' => $from, '%to' => $to[0]['email'],
	      '@message' => print_r($message, TRUE),
      ),
      WATCHDOG_NOTICE
    );

    drupal_set_message(t('mailgun test email sent from %from to %to.',
      array('%from' => $from, '%to' => $to[0]['email'])
    ), 'notice');
    return TRUE;
  }
}

/**
 * Helper to generate an array of recipients.
 *
 * @param mixed $to
 *
 * @return array
 */
function mailgun_get_to($to) {
  //@todo: extract name and add to associative array.

  $recipients = array();
  if ($to_array = explode(',', $to)) {
    foreach ($to_array as $email) {
      $recipients[] = array('email' => $email);
    }
  }
  else {
    $recipients[] = array('email' => $to);
  }
  return $recipients;
}

