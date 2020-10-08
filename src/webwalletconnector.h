//
// Created by vgulkevic on 06/10/2020.
//
#ifndef DIGITALNOTE_WEBWALLETCONNECTOR_H
#define DIGITALNOTE_WEBWALLETCONNECTOR_H

#include <string>
#include "util.h"

#include "smessage.h"

extern bool fWebWalletConnectorEnabled;

bool WebWalletConnectorStart(bool fDontStart);
bool WebWalletConnectorShutdown();
void SendUpdateToWebWallet(std::string msg);

// subscriptions
void subscribeToCoreSignals();
void unsubscribeFromCoreSignals();

#endif //DIGITALNOTE_WEBWALLETCONNECTOR_H
