#include "airdroppage.h"
#include "ui_airdroppage.h"
#include "main.h"
#include "init.h"
#include "base58.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "rpcconsole.h"
#include "smessage.h"

#include <sstream>
#include <string>
namespace fs = boost::filesystem;
AirdropPage::AirdropPage(QWidget *parent) :
        QWidget(parent),
        ui(new Ui::AirdropPage) {
    ui->setupUi(this);

    setFixedSize(900, 420);

    connect(ui->enrollButton, SIGNAL(pressed()), SLOT(enrollButtonClicked()));

    enrolledEthAddress = this->IsAlreadyEnrolledEthAddress();
    if (enrolledEthAddress != "") {
        ui->ethAddressBox->hide();
        ui->enrollButton->hide();
        std::string message = "You have already signed up for the airdrop with this address: " + enrolledEthAddress;
        ui->ethAddressLabel->setText(message.c_str());
    }
}

void AirdropPage::setModel(WalletModel *model)
{
    this->model = model;
}

AirdropPage::~AirdropPage() {
    delete ui;
}

void AirdropPage::enrollButtonClicked()
{
    enrollForAirdrop();
}

void AirdropPage::enrollForAirdrop()
{
    if (pwalletMain->IsLocked()) {
        QMessageBox::warning(this, tr("Wallet locked"), tr("Please unlock your wallet"), QMessageBox::Ok);
        return;
    }

    if (!fSecMsgEnabled) {
        QMessageBox::warning(this, tr("Secure messaging disabled"), tr("Please enabled secure messaging"), QMessageBox::Ok);
        return;
    }

    std::string ethAddress = ui->ethAddressBox->text().toUtf8().constData();
    if (ethAddress.length() != 42) {
        QMessageBox::warning(this, tr("Invalid ETH address"), tr("Please enter 42 length ETH address. Example: 0x00B54E93EE2EBA3086A55F4249873E291D1AB06C"), QMessageBox::Ok);
        return;
    }
//    ui->processing->setText("Enrolling your addresses for airdrop. Please wait...");
    ui->enrollButton->setEnabled(false);
    ui->enrollButton->hide();
    ui->ethAddressBox->hide();

    std::string sError;
    int addressesCount = SignUpForAirdrop(sError, ethAddress);

    if (addressesCount == 0) {
        QMessageBox::warning(NULL, tr("Send Secure Message"),
                             tr("Send failed: %1.").arg(sError.c_str()),
                             QMessageBox::Ok, QMessageBox::Ok);
    }

    if (addressesCount != 0) {
        AirdropPage::IsAlreadyEnrolledEthAddressWrite(ethAddress);

        std::string message = "Enrolled your " + std::to_string(addressesCount)+ " addresses \n for the airdrop with this ETH address: " + ethAddress + ".\n";
        ui->ethAddressLabel->setText(message.c_str());
    } else {
        std::string message = "Something went wrong when enrolling your addresses for the airdrop. Please try again later or check Discord for more info.";
        ui->ethAddressLabel->setText(message.c_str());
    }
}

std::string AirdropPage::IsAlreadyEnrolledEthAddress()
{
    LogPrint("airdrop", "airdrop: IsAlreadyEnrolledEthAddress");

    fs::path fullpath = GetDataDir() / "airdrop.ini";
    FILE *fp;
    errno = 0;
    if (!(fp = fopen(fullpath.string().c_str(), "r")))
    {
        return "";
    };

    char cLine[512];
    char cAddress[64];
    char *pName, *pValue;

    while (fgets(cLine, 512, fp))
    {

        cLine[strcspn(cLine, "\n")] = '\0';
        cLine[strcspn(cLine, "\r")] = '\0';
        cLine[511] = '\0'; // for safety

        // -- check that line contains a name value pair and is not a comment, or section header
        if (cLine[0] == '#' || cLine[0] == '[' || strcspn(cLine, "=") < 1)
            continue;

        if (!(pName = strtok(cLine, "="))
            || !(pValue = strtok(NULL, "=")))
            continue;

        if (strcmp(pName, "enrolledAddress") == 0)
        {
            int rv = sscanf(pValue, "%s*", cAddress);
            return std::string(cAddress);
        }
    };

    fclose(fp);

    return "";
}

bool AirdropPage::IsAlreadyEnrolledEthAddressWrite(const std::string& ethAddress)
{
    fs::path fullpath = GetDataDir() / "airdrop.ini~";

    FILE *fp;
    errno = 0;
    if (!(fp = fopen(fullpath.string().c_str(), "w")))
    {
        return false;
    };

    errno = 0;
    if (fprintf(fp, "enrolledAddress=%s\n", ethAddress.c_str()) < 0) {
        fclose(fp);
        return false;
    } else {
        fclose(fp);
    }

    try {
        fs::path finalpath = GetDataDir() / "airdrop.ini";
        fs::rename(fullpath, finalpath);
    } catch (const fs::filesystem_error& ex)
    {
    };
    return true;
}
