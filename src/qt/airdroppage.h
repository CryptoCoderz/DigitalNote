#ifndef DIGITALNOTE_AIRDROPPAGE_H
#define DIGITALNOTE_AIRDROPPAGE_H

#include "clientmodel.h"
#include "walletmodel.h"
#include "main.h"
#include "wallet.h"
#include "base58.h"
#include <QWidget>
#include "util.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTime>
#include <QTimer>
#include <QStringList>
#include <QMap>
#include <QSettings>
#include <QSlider>

namespace Ui {
    class AirdropPage;
}
class WalletModel;

class AirdropPage : public QWidget {
    Q_OBJECT
    std::string enrolledEthAddress;
public:
    explicit AirdropPage(QWidget *parent = 0);

    ~AirdropPage();

    void setModel(WalletModel *model);

public
    slots:
    void enrollButtonClicked();
    void enrollForAirdrop();

private
    slots:

private:
    Ui::AirdropPage *ui;
    WalletModel *model;
    static std::string IsAlreadyEnrolledEthAddress();
    static bool IsAlreadyEnrolledEthAddressWrite(const std::string& ethAddress);
};

#endif //DIGITALNOTE_AIRDROPPAGE_H
