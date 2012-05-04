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

#include "problemdialog.h"

#include "gui.h"
#include "scene.h"
#include "pythonlabagros.h"
#include "hermes2d/module.h"
#include "hermes2d/module_agros.h"
#include "hermes2d/coupling.h"

const int minWidth = 130;

FieldSelectDialog::FieldSelectDialog(QList<QString> fields, QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Select field"));
    setModal(true);

    m_selectedFieldId = "";

    lstFields = new QListWidget(this);
    lstFields->setIconSize(QSize(32, 32));
    lstFields->setMinimumHeight(36*8);

    std::map<QString, QString> modules = availableModules();
    for (std::map<QString, QString>::iterator it = modules.begin();
         it != modules.end(); ++it)
    {
        // add only missing fields
        if (!fields.contains(it->first))
        {
            QListWidgetItem *item = new QListWidgetItem(lstFields);
            item->setIcon(icon("fields/" + it->first));
            item->setText(it->second);
            item->setData(Qt::UserRole, it->first);

            lstFields->addItem(item);
        }
    }

    connect(lstFields, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
            this, SLOT(doItemDoubleClicked(QListWidgetItem *)));
    connect(lstFields, SIGNAL(itemActivated(QListWidgetItem *)),
            this, SLOT(doItemSelected(QListWidgetItem *)));
    connect(lstFields, SIGNAL(itemPressed(QListWidgetItem *)),
            this, SLOT(doItemSelected(QListWidgetItem *)));

    QGridLayout *layoutSurface = new QGridLayout();
    layoutSurface->addWidget(lstFields);

    QWidget *widget = new QWidget();
    widget->setLayout(layoutSurface);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(doAccept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(doReject()));

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(widget, 1);
    layout->addStretch();
    layout->addWidget(buttonBox);

    setLayout(layout);

    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    if (lstFields->count() > 0)
    {
        lstFields->setCurrentRow(0);
        doItemSelected(lstFields->currentItem());
    }
}

void FieldSelectDialog::doAccept()
{
    accept();
}

void FieldSelectDialog::doReject()
{
    reject();
}

int FieldSelectDialog::showDialog()
{
    return exec();
}

void FieldSelectDialog::doItemSelected(QListWidgetItem *item)
{
    m_selectedFieldId = item->data(Qt::UserRole).toString();
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void FieldSelectDialog::doItemDoubleClicked(QListWidgetItem *item)
{
    if (lstFields->currentItem())
    {
        m_selectedFieldId = lstFields->currentItem()->data(Qt::UserRole).toString();
        accept();
    }
}

// ********************************************************************************************************

FieldWidget::FieldWidget(FieldInfo *fieldInfo, QWidget *parent)
    : QWidget(parent), m_fieldInfo(fieldInfo)
{
    createContent();
    load();
}

void FieldWidget::createContent()
{
    /*
    QLabel *lblField = new QLabel();
    QPixmap pixmap;
    pixmap.load(QString(":/fields/%1.png").arg(m_fieldInfo->fieldId()));
    lblField->setPixmap(pixmap.scaled(QSize(48, 48), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    */

    // equations
    lblEquationPixmap = new QLabel("");

    cmbAdaptivityType = new QComboBox();
    txtAdaptivitySteps = new QSpinBox(this);
    txtAdaptivitySteps->setMinimum(1);
    txtAdaptivitySteps->setMaximum(100);
    txtAdaptivityTolerance = new SLineEditDouble(1);

    // mesh
    txtNumberOfRefinements = new QSpinBox(this);
    txtNumberOfRefinements->setMinimum(0);
    txtNumberOfRefinements->setMaximum(5);
    txtPolynomialOrder = new QSpinBox(this);
    txtPolynomialOrder->setMinimum(1);
    txtPolynomialOrder->setMaximum(10);

    // weak forms
    cmbWeakForms = new QComboBox();

    // transient
    cmbAnalysisType = new QComboBox();
    txtTransientInitialCondition = new ValueLineEdit();

    // linearity
    cmbLinearityType = new QComboBox();
    txtNonlinearSteps = new QSpinBox(this);
    txtNonlinearSteps->setMinimum(1);
    txtNonlinearSteps->setMaximum(100);
    txtNonlinearTolerance = new SLineEditDouble(1);

    connect(cmbAdaptivityType, SIGNAL(currentIndexChanged(int)), this, SLOT(doAdaptivityChanged(int)));
    connect(cmbAnalysisType, SIGNAL(currentIndexChanged(int)), this, SLOT(doAnalysisTypeChanged(int)));
    //connect(cmbAnalysisType, SIGNAL(currentIndexChanged(int)), m_problemDialog, SLOT(doFindCouplings()));

    connect(cmbLinearityType, SIGNAL(currentIndexChanged(int)), this, SLOT(doLinearityTypeChanged(int)));

    // fill combobox
    fillComboBox();

    // table
    QGridLayout *layoutGeneral = new QGridLayout();
    layoutGeneral->setColumnMinimumWidth(0, minWidth);
    layoutGeneral->setColumnStretch(1, 1);
    layoutGeneral->addWidget(new QLabel(tr("Type of analysis:")), 0, 0);
    layoutGeneral->addWidget(cmbAnalysisType, 0, 1);
    layoutGeneral->addWidget(new QLabel(tr("Weak forms:")), 2, 0);
    layoutGeneral->addWidget(cmbWeakForms, 2, 1);

    QGroupBox *grpGeneral = new QGroupBox(tr("General"));
    grpGeneral->setLayout(layoutGeneral);

    // transient analysis
    QGridLayout *layoutTransientAnalysis = new QGridLayout();
    layoutTransientAnalysis->setColumnMinimumWidth(0, minWidth);
    layoutTransientAnalysis->setColumnStretch(1, 1);
    layoutTransientAnalysis->addWidget(new QLabel(tr("Initial condition:")), 0, 0);
    layoutTransientAnalysis->addWidget(txtTransientInitialCondition, 0, 1);

    QGroupBox *grpTransientAnalysis = new QGroupBox(tr("Transient analysis"));
    grpTransientAnalysis->setLayout(layoutTransientAnalysis);

    // harmonic analysis
    QGridLayout *layoutMesh = new QGridLayout();
    layoutMesh->setColumnMinimumWidth(0, minWidth);
    layoutMesh->setColumnStretch(1, 1);
    layoutMesh->addWidget(new QLabel(tr("Number of refinements:")), 0, 0);
    layoutMesh->addWidget(txtNumberOfRefinements, 0, 1);
    layoutMesh->addWidget(new QLabel(tr("Polynomial order:")), 1, 0);
    layoutMesh->addWidget(txtPolynomialOrder, 1, 1);

    QGroupBox *grpMesh = new QGroupBox(tr("Mesh parameters"));
    grpMesh->setLayout(layoutMesh);

    // adaptivity
    QGridLayout *layoutAdaptivity = new QGridLayout();
    layoutAdaptivity->setColumnMinimumWidth(0, minWidth);
    layoutAdaptivity->setColumnStretch(1, 1);
    layoutAdaptivity->addWidget(new QLabel(tr("Type:")), 0, 0);
    layoutAdaptivity->addWidget(cmbAdaptivityType, 0, 1);
    layoutAdaptivity->addWidget(new QLabel(tr("Steps:")), 1, 0);
    layoutAdaptivity->addWidget(txtAdaptivitySteps, 1, 1);
    layoutAdaptivity->addWidget(new QLabel(tr("Tolerance (%):")), 2, 0);
    layoutAdaptivity->addWidget(txtAdaptivityTolerance, 2, 1);

    QGroupBox *grpAdaptivity = new QGroupBox(tr("Adaptivity"));
    grpAdaptivity->setLayout(layoutAdaptivity);

    // linearity
    QGridLayout *layoutLinearity = new QGridLayout();
    layoutLinearity->setColumnMinimumWidth(0, minWidth);
    layoutLinearity->setColumnStretch(1, 1);
    layoutLinearity->addWidget(new QLabel(tr("Linearity:")), 0, 0);
    layoutLinearity->addWidget(cmbLinearityType, 0, 1);
    layoutLinearity->addWidget(new QLabel(tr("Tolerance (%):")), 1, 0);
    layoutLinearity->addWidget(txtNonlinearTolerance, 1, 1);
    layoutLinearity->addWidget(new QLabel(tr("Steps:")), 2, 0);
    layoutLinearity->addWidget(txtNonlinearSteps, 2, 1);

    QGroupBox *grpLinearity = new QGroupBox(tr("Solver"));
    grpLinearity->setLayout(layoutLinearity);

    // left
    QVBoxLayout *layoutLeft = new QVBoxLayout();
    layoutLeft->addWidget(grpGeneral);
    layoutLeft->addWidget(grpLinearity);
    layoutLeft->addStretch();

    // right
    QVBoxLayout *layoutRight = new QVBoxLayout();
    layoutRight->addWidget(grpMesh);
    layoutRight->addWidget(grpAdaptivity);
    layoutRight->addWidget(grpTransientAnalysis);
    layoutRight->addStretch();

    // both
    QHBoxLayout *layoutPanel = new QHBoxLayout();
    layoutPanel->addLayout(layoutLeft);
    layoutPanel->addLayout(layoutRight);

    // equation
    QVBoxLayout *layoutEquation = new QVBoxLayout();
    layoutEquation->addWidget(lblEquationPixmap);

    QGroupBox *grpEquation = new QGroupBox(tr("Partial differential equation"));
    grpEquation->setLayout(layoutEquation);

    QVBoxLayout *layoutProblem = new QVBoxLayout();
    layoutProblem->addWidget(grpEquation);
    layoutProblem->addLayout(layoutPanel);

    setLayout(layoutProblem);

    setMinimumSize(sizeHint());
}

void FieldWidget::fillComboBox()
{
    cmbAdaptivityType->clear();
    cmbAdaptivityType->addItem(adaptivityTypeString(AdaptivityType_None), AdaptivityType_None);
    cmbAdaptivityType->addItem(adaptivityTypeString(AdaptivityType_H), AdaptivityType_H);
    cmbAdaptivityType->addItem(adaptivityTypeString(AdaptivityType_P), AdaptivityType_P);
    cmbAdaptivityType->addItem(adaptivityTypeString(AdaptivityType_HP), AdaptivityType_HP);

    cmbWeakForms->clear();
    cmbWeakForms->addItem(weakFormsTypeString(WeakFormsType_Compiled), WeakFormsType_Compiled);
    cmbWeakForms->addItem(weakFormsTypeString(WeakFormsType_Interpreted), WeakFormsType_Interpreted);

    cmbLinearityType->addItem(linearityTypeString(LinearityType_Linear), LinearityType_Linear);
    // if (hermesField->hasNonlinearity())
    {
        cmbLinearityType->addItem(linearityTypeString(LinearityType_Picard), LinearityType_Picard);
        cmbLinearityType->addItem(linearityTypeString(LinearityType_Newton), LinearityType_Newton);
    }

    std::map<QString, QString> analyses = availableAnalyses(m_fieldInfo->fieldId());
    for (std::map<QString, QString>::iterator it = analyses.begin(); it != analyses.end(); ++it)
        cmbAnalysisType->addItem(it->second, analysisTypeFromStringKey(it->first));
}

void FieldWidget::load()
{
    // analysis type
    cmbAnalysisType->setCurrentIndex(cmbAnalysisType->findData(m_fieldInfo->analysisType()));
    if (cmbAnalysisType->currentIndex() == -1)
        cmbAnalysisType->setCurrentIndex(0);
    // adaptivity
    cmbAdaptivityType->setCurrentIndex(cmbAdaptivityType->findData(m_fieldInfo->adaptivityType()));
    txtAdaptivitySteps->setValue(m_fieldInfo->adaptivitySteps());
    txtAdaptivityTolerance->setValue(m_fieldInfo->adaptivityTolerance());
    // weakforms
    cmbWeakForms->setCurrentIndex(cmbWeakForms->findData(m_fieldInfo->weakFormsType()));
    //mesh
    txtNumberOfRefinements->setValue(m_fieldInfo->numberOfRefinements());
    txtPolynomialOrder->setValue(m_fieldInfo->polynomialOrder());
    // transient
    txtTransientInitialCondition->setValue(m_fieldInfo->initialCondition());
    // linearity
    cmbLinearityType->setCurrentIndex(cmbLinearityType->findData(m_fieldInfo->linearityType()));
    txtNonlinearSteps->setValue(m_fieldInfo->nonlinearSteps());
    txtNonlinearTolerance->setValue(m_fieldInfo->nonlinearTolerance());

    doAnalysisTypeChanged(cmbAnalysisType->currentIndex());
}

bool FieldWidget::save()
{    
    m_fieldInfo->setAnalysisType((AnalysisType) cmbAnalysisType->itemData(cmbAnalysisType->currentIndex()).toInt());

    // adaptivity
    m_fieldInfo->setAdaptivityType((AdaptivityType) cmbAdaptivityType->itemData(cmbAdaptivityType->currentIndex()).toInt());
    m_fieldInfo->setAdaptivitySteps(txtAdaptivitySteps->value());
    m_fieldInfo->setAdaptivityTolerance(txtAdaptivityTolerance->value());
    // weakforms
    m_fieldInfo->setWeakFormsType((WeakFormsType) cmbWeakForms->itemData(cmbWeakForms->currentIndex()).toInt());
    //mesh
    m_fieldInfo->setNumberOfRefinements(txtNumberOfRefinements->value());
    m_fieldInfo->setPolynomialOrder(txtPolynomialOrder->value());
    // transient
    m_fieldInfo->setInitialCondition(txtTransientInitialCondition->value());
    // linearity
    m_fieldInfo->setLinearityType((LinearityType) cmbLinearityType->itemData(cmbLinearityType->currentIndex()).toInt());
    m_fieldInfo->setNonlinearSteps(txtNonlinearSteps->value());
    m_fieldInfo->setNonlinearTolerance(txtNonlinearTolerance->value());

    return true;
}

void FieldWidget::refresh()
{
    doAnalysisTypeChanged(cmbAnalysisType->currentIndex());
}

FieldInfo *FieldWidget::fieldInfo()
{
    return m_fieldInfo;
}

void FieldWidget::doAnalysisTypeChanged(int index)
{
    txtTransientInitialCondition->setEnabled((AnalysisType) cmbAnalysisType->itemData(index).toInt() == AnalysisType_Transient);

    doShowEquation();
}

void FieldWidget::doShowEquation()
{
    readPixmap(lblEquationPixmap,
               QString(":/equations/%1/%1_%2.png")
               .arg(m_fieldInfo->fieldId())
               .arg(analysisTypeToStringKey((AnalysisType) cmbAnalysisType->itemData(cmbAnalysisType->currentIndex()).toInt())));
}

void FieldWidget::doAdaptivityChanged(int index)
{
    txtAdaptivitySteps->setEnabled((AdaptivityType) cmbAdaptivityType->itemData(index).toInt() != AdaptivityType_None);
    txtAdaptivityTolerance->setEnabled((AdaptivityType) cmbAdaptivityType->itemData(index).toInt() != AdaptivityType_None);
}

void FieldWidget::doLinearityTypeChanged(int index)
{
    txtNonlinearSteps->setEnabled((LinearityType) cmbLinearityType->itemData(index).toInt() != LinearityType_Linear);
    txtNonlinearTolerance->setEnabled((LinearityType) cmbLinearityType->itemData(index).toInt() != LinearityType_Linear);
}

// ********************************************************************************************

FieldDialog::FieldDialog(FieldInfo *fieldInfo, QWidget *parent) : QDialog(parent)
{
    setWindowTitle(fieldInfo->module()->name);

    fieldWidget = new FieldWidget(fieldInfo, this);

    // dialog buttons
    QPushButton *btnDeleteField = new QPushButton(tr("Delete field"));
    btnDeleteField->setDefault(false);
    btnDeleteField->setEnabled(Util::problem()->hasField(fieldInfo->fieldId()));
    connect(btnDeleteField, SIGNAL(clicked()), this, SLOT(deleteField()));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->addButton(btnDeleteField, QDialogButtonBox::ActionRole);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(doAccept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(fieldWidget);
    layout->addStretch();
    layout->addWidget(buttonBox);

    setLayout(layout);

    setMaximumSize(sizeHint());

    QSettings settings;
    restoreGeometry(settings.value("FieldDialog/Geometry", saveGeometry()).toByteArray());
}

FieldDialog::~FieldDialog()
{
    QSettings settings;
    settings.setValue("FieldDialog/Geometry", saveGeometry());
}

void FieldDialog::doAccept()
{
    fieldWidget->save();
    accept();
}

void FieldDialog::deleteField()
{
    if (QMessageBox::question(this, tr("Delete"), tr("Physical field '%1' will be pernamently deleted. Are you sure?").
                              arg(fieldWidget->fieldInfo()->module()->name), tr("&Yes"), tr("&No")) == 0)
    {
        Util::problem()->removeField(fieldWidget->fieldInfo());
        accept();
    }
}

// ********************************************************************************************

FieldsToobar::FieldsToobar(QWidget *parent) : QWidget(parent)
{
    createControls();

    connect(Util::problem(), SIGNAL(fieldsChanged()), this, SLOT(refresh()));
    connect(Util::scene(), SIGNAL(invalidated()), this, SLOT(refresh()));

    refresh();
}

void FieldsToobar::createControls()
{
    actFieldsGroup = new QActionGroup(this);
    connect(actFieldsGroup, SIGNAL(triggered(QAction *)), this, SLOT(fieldDialog(QAction *)));

    tlbFields = new QToolBar(this);
    tlbFields->setIconSize(QSize(36, 36));
    tlbFields->setOrientation(Qt::Horizontal);
    tlbFields->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    tlbFields->setStyleSheet("QToolButton { font-size: 8pt; }");

    // dialog buttons
    QPushButton *btnAddField = new QPushButton(tr("Add field")); // icon("tabadd")
    connect(btnAddField, SIGNAL(clicked()), SLOT(addField()));

    QHBoxLayout *layoutButtons = new QHBoxLayout();
    layoutButtons->addStretch();
    layoutButtons->addWidget(btnAddField);

    QVBoxLayout *layoutToolBar = new QVBoxLayout();
    layoutToolBar->setContentsMargins(0, 1, 0, 0);
    layoutToolBar->addWidget(tlbFields);
    layoutToolBar->addLayout(layoutButtons);
    layoutToolBar->addStretch();

    setLayout(layoutToolBar);
}

void FieldsToobar::refresh()
{
    // fields
    tlbFields->clear();
    actFieldsGroup->actions().clear();

    foreach (FieldInfo *fieldInfo, Util::problem()->fieldInfos())
    {
        QAction *actField = new QAction(fieldInfo->module() ? fieldInfo->module()->name : fieldInfo->fieldId(), this);
        actField->setIcon(icon("fields/" + fieldInfo->fieldId()));
        actField->setData(fieldInfo->fieldId());

        actFieldsGroup->addAction(actField);
        tlbFields->addAction(actField);
    }
    // spacing
    QLabel *spacing = new QLabel;
    spacing->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tlbFields->addWidget(spacing);
}

void FieldsToobar::fieldDialog(QAction *action)
{
    FieldInfo *fieldInfo = Util::problem()->fieldInfo(action->data().toString());
    if (fieldInfo)
    {
        FieldDialog fieldDialog(fieldInfo, this);
        if (fieldDialog.exec() == QDialog::Accepted)
            refresh();
    }
}

void FieldsToobar::addField()
{
    // select field dialog
    FieldSelectDialog dialog(Util::problem()->fieldInfos().keys(), this);
    if (dialog.showDialog() == QDialog::Accepted)
    {
        // add field
        FieldInfo *fieldInfo = new FieldInfo(dialog.selectedFieldId());

        FieldDialog fieldDialog(fieldInfo, this);
        if (fieldDialog.exec() == QDialog::Accepted)
        {
            Util::problem()->addField(fieldInfo);

            refresh();
        }
        else
        {
            delete fieldInfo;
        }
    }
}

// ********************************************************************************************

CouplingsWidget::CouplingsWidget(QWidget *parent) : QWidget(parent)
{
    Util::problem()->synchronizeCouplings();
    m_couplingInfos = Util::problem()->couplingInfos();

    layoutTable = NULL;
    createContent();

    connect(Util::problem(), SIGNAL(fieldsChanged()), this, SLOT(refresh()));

    load();
}

void CouplingsWidget::createContent()
{
    if (layoutTable)
    {
        save();

        delete layoutTable;
        qDeleteAll(this->children());
    }

    layoutTable = new QGridLayout();
    layoutTable->setColumnMinimumWidth(0, minWidth);
    layoutTable->setColumnStretch(1, 1);

    m_comboBoxes.clear();
    int line = 0;
    foreach (CouplingInfo *couplingInfo, m_couplingInfos)
    {
        layoutTable->addWidget(new QLabel(/*tr(*/QString::fromStdString(couplingInfo->coupling()->name)/*)*/), line, 0);
        m_comboBoxes[couplingInfo] = new QComboBox();
        layoutTable->addWidget(m_comboBoxes[couplingInfo], line, 1);
        line++;
    }

    fillComboBox();
    load();

    setLayout(layoutTable);
}

void CouplingsWidget::fillComboBox()
{
    foreach (QComboBox* comboBox, m_comboBoxes)
    {
        comboBox->addItem(couplingTypeString(CouplingType_None), CouplingType_None);
        comboBox->addItem(couplingTypeString(CouplingType_Weak), CouplingType_Weak);
        comboBox->addItem(couplingTypeString(CouplingType_Hard), CouplingType_Hard);
    }
}

void CouplingsWidget::load()
{
    foreach (CouplingInfo *couplingInfo, m_couplingInfos)
    {
        m_comboBoxes[couplingInfo]->setCurrentIndex(couplingInfo->couplingType());
    }
}

void CouplingsWidget::save()
{
    foreach(CouplingInfo *couplingInfo, m_couplingInfos)
    {
        if(m_comboBoxes.contains(couplingInfo))
        {
            couplingInfo->setCouplingType((CouplingType) m_comboBoxes[couplingInfo]->itemData(m_comboBoxes[couplingInfo]->currentIndex()).toInt());
        }
    }

    Util::problem()->setCouplingInfos(m_couplingInfos);
}

void CouplingsWidget::refresh()
{
    m_couplingInfos = Util::problem()->couplingInfos();
    CouplingInfo::synchronizeCouplings(Util::problem()->fieldInfos(), m_couplingInfos);

    createContent();
}

// ********************************************************************************************

ProblemWidget::ProblemWidget(QWidget *parent) : QWidget(parent)
{
    createActions();
    createControls();

    updateControls();

    connect(Util::scene(), SIGNAL(invalidated()), this, SLOT(updateControls()));

    setMinimumSize(sizeHint());
}

void ProblemWidget::createActions()
{
    actProperties = new QAction(icon("document-properties"), tr("Properties"), this);
    actProperties->setShortcut(tr("F12"));
    actProperties->setStatusTip(tr("Problem properties"));
    actProperties->setCheckable(true);
}

void ProblemWidget::createControls()
{
    // tab
    QTabWidget *tabType = new QTabWidget();
    tabType->addTab(createControlsGeneral(), icon(""), tr("General"));
    tabType->addTab(createControlsScriptAndDescription(), icon(""), tr("Script and description"));

    // fields toolbar
    fieldsToolbar = new FieldsToobar(this);
    QVBoxLayout *layoutFields = new QVBoxLayout();
    layoutFields->addWidget(fieldsToolbar);
    layoutFields->addStretch();

    QGroupBox *grpFieldsToolbar = new QGroupBox(tr("Physical fields"));
    grpFieldsToolbar->setLayout(layoutFields);

    // couplings
    couplingsWidget = new CouplingsWidget(this);

    QVBoxLayout *layoutCouplings = new QVBoxLayout();
    layoutCouplings->addWidget(couplingsWidget);

    grpCouplings = new QGroupBox(tr("Couplings"));
    grpCouplings->setLayout(layoutCouplings);

    // dialog buttons
    QPushButton *btnOK = new QPushButton(tr("Apply"));
    connect(btnOK, SIGNAL(clicked()), SLOT(doApply()));

    QHBoxLayout *layoutButtons = new QHBoxLayout();
    layoutButtons->addStretch();
    layoutButtons->addWidget(btnOK);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(tabType);
    layout->addWidget(grpFieldsToolbar);
    layout->addWidget(grpCouplings);
    layout->addStretch();
    layout->addLayout(layoutButtons);

    setLayout(layout);
}

QWidget *ProblemWidget::createControlsGeneral()
{
    // problem
    cmbCoordinateType = new QComboBox();
    txtName = new QLineEdit("");

    // matrix solver
    cmbMatrixSolver = new QComboBox();

    // mesh type
    cmbMeshType = new QComboBox();

    // harmonic
    txtFrequency = new SLineEditDouble();

    // transient
    txtTransientTimeStep = new ValueLineEdit();
    txtTransientTimeTotal = new ValueLineEdit();
    lblTransientSteps = new QLabel("0");

    connect(txtTransientTimeStep, SIGNAL(editingFinished()), this, SLOT(doTransientChanged()));
    connect(txtTransientTimeTotal, SIGNAL(editingFinished()), this, SLOT(doTransientChanged()));

    // fill combobox
    fillComboBox();

    // general
    QGridLayout *layoutGeneral = new QGridLayout();
    layoutGeneral->setColumnMinimumWidth(0, minWidth);
    layoutGeneral->setColumnStretch(1, 1);
    layoutGeneral->addWidget(new QLabel(tr("Coordinate type:")), 0, 0);
    layoutGeneral->addWidget(cmbCoordinateType, 0, 1);
    layoutGeneral->addWidget(new QLabel(tr("Linear solver:")), 1, 0);
    layoutGeneral->addWidget(cmbMatrixSolver, 1, 1);
    layoutGeneral->addWidget(new QLabel(tr("Mesh type:")), 2, 0);
    layoutGeneral->addWidget(cmbMeshType, 2, 1);

    QGroupBox *grpGeneral = new QGroupBox(tr("General"));
    grpGeneral->setLayout(layoutGeneral);

    // harmonic analysis
    QGridLayout *layoutHarmonicAnalysis = new QGridLayout();
    layoutHarmonicAnalysis->setColumnMinimumWidth(0, minWidth);
    layoutHarmonicAnalysis->addWidget(new QLabel(tr("Frequency (Hz):")), 0, 0);
    layoutHarmonicAnalysis->addWidget(txtFrequency, 0, 1);

    QGroupBox *grpHarmonicAnalysis = new QGroupBox(tr("Harmonic analysis"));
    grpHarmonicAnalysis->setLayout(layoutHarmonicAnalysis);

    // harmonic analysis
    QGridLayout *layoutTransientAnalysis = new QGridLayout();
    layoutTransientAnalysis->setColumnMinimumWidth(0, minWidth);
    layoutTransientAnalysis->setColumnStretch(1, 1);
    layoutTransientAnalysis->addWidget(new QLabel(tr("Time step (s):")), 0, 0);
    layoutTransientAnalysis->addWidget(txtTransientTimeStep, 0, 1);
    layoutTransientAnalysis->addWidget(new QLabel(tr("Total time (s):")), 1, 0);
    layoutTransientAnalysis->addWidget(txtTransientTimeTotal, 1, 1);
    layoutTransientAnalysis->addWidget(new QLabel(tr("Steps:")), 2, 0);
    layoutTransientAnalysis->addWidget(lblTransientSteps, 2, 1);

    QGroupBox *grpTransientAnalysis = new QGroupBox(tr("Transient analysis"));
    grpTransientAnalysis->setLayout(layoutTransientAnalysis);

    // both
    QVBoxLayout *layoutPanel = new QVBoxLayout();
    layoutPanel->addWidget(grpGeneral);
    layoutPanel->addWidget(grpHarmonicAnalysis);
    layoutPanel->addWidget(grpTransientAnalysis);
    layoutPanel->addStretch();

    // name
    QGridLayout *layoutName = new QGridLayout();
    layoutName->addWidget(new QLabel(tr("Name:")), 0, 0);
    layoutName->addWidget(txtName, 0, 1);

    QVBoxLayout *layoutProblem = new QVBoxLayout();
    layoutProblem->addLayout(layoutName);
    layoutProblem->addLayout(layoutPanel);
    layoutProblem->addStretch();

    QWidget *widMain = new QWidget();
    widMain->setLayout(layoutProblem);

    return widMain;
}

QWidget *ProblemWidget::createControlsScriptAndDescription()
{
    // startup script
    txtStartupScript = new ScriptEditor(currentPythonEngine(), this);

    QVBoxLayout *layoutStartup = new QVBoxLayout();
    layoutStartup->addWidget(txtStartupScript);

    QGroupBox *grpStartup = new QGroupBox(tr("Startup script"));
    grpStartup->setLayout(layoutStartup);

    // description
    txtDescription = new QTextEdit(this);
    txtDescription->setAcceptRichText(false);

    QVBoxLayout *layoutDescription = new QVBoxLayout();
    layoutDescription->addWidget(txtDescription);

    QGroupBox *grpDescription = new QGroupBox(tr("Description"));
    grpDescription->setLayout(layoutDescription);

    // layout
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(grpStartup, 2);
    layout->addWidget(grpDescription, 1);
    layout->addStretch();

    QWidget *widget = new QWidget();
    widget->setLayout(layout);

    return widget;
}

void ProblemWidget::fillComboBox()
{
    cmbCoordinateType->clear();
    cmbCoordinateType->addItem(coordinateTypeString(CoordinateType_Planar), CoordinateType_Planar);
    cmbCoordinateType->addItem(coordinateTypeString(CoordinateType_Axisymmetric), CoordinateType_Axisymmetric);

    cmbMeshType->addItem(meshTypeString(MeshType_Triangle), MeshType_Triangle);
    cmbMeshType->addItem(meshTypeString(MeshType_QuadFineDivision), MeshType_QuadFineDivision);
    cmbMeshType->addItem(meshTypeString(MeshType_QuadRoughDivision), MeshType_QuadRoughDivision);
    cmbMeshType->addItem(meshTypeString(MeshType_QuadJoin), MeshType_QuadJoin);

    cmbMatrixSolver->addItem(matrixSolverTypeString(Hermes::SOLVER_UMFPACK), Hermes::SOLVER_UMFPACK);
}

void ProblemWidget::updateControls()
{
    // main
    txtName->setText(Util::problem()->config()->name());
    cmbCoordinateType->setCurrentIndex(cmbCoordinateType->findData(Util::problem()->config()->coordinateType()));
    if (cmbCoordinateType->currentIndex() == -1)
        cmbCoordinateType->setCurrentIndex(0);

    // mesh type
    cmbMeshType->setCurrentIndex(cmbMeshType->findData(Util::problem()->config()->meshType()));
    // harmonic magnetic
    txtFrequency->setValue(Util::problem()->config()->frequency());
    // transient
    txtTransientTimeStep->setValue(Util::problem()->config()->timeStep());
    txtTransientTimeTotal->setValue(Util::problem()->config()->timeTotal());
    // matrix solver
    cmbMatrixSolver->setCurrentIndex(cmbMatrixSolver->findData(Util::problem()->config()->matrixSolver()));
    // startup
    txtStartupScript->setPlainText(Util::problem()->config()->startupscript());
    // description
    txtDescription->setPlainText(Util::problem()->config()->description());

    // couplings
    fieldsToolbar->refresh();
    couplingsWidget->refresh();

    grpCouplings->setVisible(couplingsWidget->count() > 0);

    doTransientChanged();
}

bool ProblemWidget::save()
{
    // save properties
    // Util::problem()->config()->blockSignals(true);

    Util::problem()->config()->setName(txtName->text());
    Util::problem()->config()->setCoordinateType((CoordinateType) cmbCoordinateType->itemData(cmbCoordinateType->currentIndex()).toInt());
    Util::problem()->config()->setMeshType((MeshType) cmbMeshType->itemData(cmbMeshType->currentIndex()).toInt());

    Util::problem()->config()->setFrequency(txtFrequency->value());

    Util::problem()->config()->setTimeStep(txtTransientTimeStep->value());
    Util::problem()->config()->setTimeTotal(txtTransientTimeTotal->value());

    Util::problem()->config()->setDescription(txtDescription->toPlainText());

    // matrix solver
    Util::problem()->config()->setMatrixSolver((Hermes::MatrixSolverType) cmbMatrixSolver->itemData(cmbMatrixSolver->currentIndex()).toInt());

    // Util::problem()->config()->blockSignals(false);
    Util::problem()->config()->refresh();

    // save couplings
    couplingsWidget->save();

    // run and check startup script
    if (!txtStartupScript->toPlainText().isEmpty())
    {
        ScriptResult scriptResult = runPythonScript(txtStartupScript->toPlainText());
        if (scriptResult.isError)
        {
            QMessageBox::critical(QApplication::activeWindow(), QObject::tr("Error"), scriptResult.text);
            return false;
        }
        else
        {
            Util::problem()->config()->setStartupScript(txtStartupScript->toPlainText());
        }
    }

    return true;
}

void ProblemWidget::doApply()
{
    save();
}

void ProblemWidget::doOpenXML()
{
    QString fileName;
    //TODO custom
    //    if (cmbPhysicField->itemData(cmbPhysicField->currentIndex()).toString() == "custom")
    //    {
    //        fileName = Util::problem()->config()->fileName.left(Util::problem()->config()->fileName.size() - 4) + ".xml";

    //        if (!QFile::exists(fileName))
    //            if (QMessageBox::question(this, tr("Custom module file"), tr("Custom module doesn't exist. Could I create it?"),
    //                                      QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    //            {
    //                // copy custom module
    //                QFile::copy(datadir() + "/resources/custom.xml",
    //                            fileName);
    //            }
    //    }
    //    else
    //    {
    //        fileName = datadir() + "/modules/" + cmbPhysicField->itemData(cmbPhysicField->currentIndex()).toString() + ".xml";
    //    }

    //    if (QFile::exists(fileName))
    //        QDesktopServices::openUrl(QUrl(fileName));
}

void ProblemWidget::doTransientChanged()
{
    if (txtTransientTimeStep->evaluate(true) &&
            txtTransientTimeTotal->evaluate(true))
    {
        lblTransientSteps->setText(QString("%1").arg(floor(txtTransientTimeTotal->number()/txtTransientTimeStep->number())));
    }
}

