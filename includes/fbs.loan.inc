<?php

/**
 * @file
 * Handles user loans.
 */

/**
 * Implements provider loan, list.
 *
 * @param object $account
 *   User to fetch list for.
 *
 * @return array
 *   Loan information.
 */
function fbs_loan_list($account) {
  $result = array();

  $res = fbs_service()->MaterialLoans->getLoans(fbs_service()->agencyId, fbs_patron_id($account));

  foreach ($res as $loan) {
    $id = $loan->loanDetails->loanId;

    $loan_data = array(
      'ding_entity_id' => ding_provider_build_entity_id($loan->loanDetails->recordId),
      'loan_date' => $loan->loanDetails->loanDate,
      'expiry' => $loan->loanDetails->dueDate,
      'renewable' => $loan->isRenewable,
      'materials_number' => $loan->loanDetails->materialItemNumber,
    );

    // If this is a periodical, add in issue data.
    if (isset($loan->loanDetails->periodical)) {
      $periodical = $loan->loanDetails->periodical;
      $vol = $periodical->volume;
      if (!empty($periodical->volumeNumber)) {
        $vol .= '.' . $periodical->volumeNumber;
      }
      if (!empty($periodical->volumeYear)) {
        $loan_data['notes'] = t('Issue @vol, @year', array('@vol' => $vol, '@year' => $periodical->volumeYear));
      }
      else {
        $loan_data['notes'] = t('Issue @vol', array('@vol' => $vol));
      }
    }

    // Handle inter library loans.
    if (!empty($loan->ilBibliographicRecord)) {
      $loan_data['display_name'] = $loan->ilBibliographicRecord->title;
    }

    $result[$id] = new DingProviderLoan($id, $loan_data);
  }

  return $result;
}

/**
 * Implements provider loan, renew.
 *
 * @param object $account
 *   User to renew loan for.
 * @param array $loan
 *   Loan ids to renew.
 *
 * @return array
 *   Result of revewals.
 */
function fbs_loan_renew($account, $loans) {
  $result = array();

  $res = fbs_service()->MaterialLoans->renewLoans(fbs_service()->agencyId, fbs_patron_id($account), $loans);

  foreach ($res as $loan) {
    // Using an array for renewalStatus is an odd choice, but we'll only
    // consider the loan renewed if 'renewed' is the only status.
    if ($loan->renewalStatus == array('renewed')) {
      $result[$loan->loanDetails->loanId] = DingProviderLoan::STATUS_RENEWED;
    }
    else {
      $result[$loan->loanDetails->loanId] = DingProviderLoan::STATUS_NOT_RENEWED;
    }
  }

  return $result;
}
