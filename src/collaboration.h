// This file is part of Agros2D.
//
// Agros2D is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Agros2D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Agros2D.  If not, see <http://www.gnu.org/licenses/>.
//
// hp-FEM group (http://hpfem.org/)
// University of Nevada, Reno (UNR) and University of West Bohemia, Pilsen
// Email: agros2d@googlegroups.com, home page: http://hpfem.org/agros2d/

#ifndef COLABORATION_H
#define COLABORATION_H

#include "util.h"
#include "collaboration.h"

#include <QSvgWidget>

class QWebView;

class ServerDownloadDialog : public QDialog
{
    Q_OBJECT
public:
    ServerDownloadDialog(QWidget *parent = 0);
    ~ServerDownloadDialog();

    int showDialog();
    void createControls();
    void readFromServerXML(int ID, int version);

    inline QString fileName() { return m_fileName; }

private slots:
    void load(const QString &str);
    void doClose();
    void httpContentFinished();
    void httpFileFinished();
    void linkClicked(const QUrl &url);

private:
    QString m_fileName;

    QWebView *webView;

    QNetworkAccessManager networkAccessManager;
    QNetworkReply *networkReply;
};

class ServerUploadDialog : public QDialog
{
    Q_OBJECT
public:
    ServerUploadDialog(QWidget *parent = 0);
    ~ServerUploadDialog();

    int showDialog();
    void createControls();
    void readFromServerContent();
    void uploadToServer();

private slots:
    void doUpload();
    void doClose();
    void httpContentFinished();
    void httpFileFinished();
    void doDocumentChanged();
    void doExistingProblemSelected(int index);

private:
    QNetworkAccessManager networkAccessManager;
    QNetworkReply *networkReply;

    QRadioButton *radDocumentNew;
    QRadioButton *radDocumentExisting;
    QLabel *lblName;
    QLabel *lblInformation;
    QComboBox *cmbName;
    QLineEdit *txtAuthor;
    QLineEdit *txtAffiliation;
    QLabel *lblNotification;
    QPushButton *btnUpload;
};

#endif // COLABORATION_H
