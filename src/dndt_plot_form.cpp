/*****************************************************************************
 **                                                                         **
 **  This file is part of GlassPlotter.                                     **
 **                                                                         **
 **  GlassPlotter is free software: you can redistribute it and/or modify   **
 **  it under the terms of the GNU General Public License as published by   **
 **  the Free Software Foundation, either version 3 of the License, or      **
 **  (at your option) any later version.                                    **
 **                                                                         **
 **  GlassPlotter is distributed in the hope that it will be useful,        **
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of         **
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          **
 **  GNU General Public License for more details.                           **
 **                                                                         **
 **  You should have received a copy of the GNU General Public License      **
 **  along with GlassPlotter.  If not, see <http://www.gnu.org/licenses/>.  **
 **                                                                         **
 *****************************************************************************
 **  Author  : Hiiragi                                                      **
 **  Contact : heterophyllus.work@gmail.com                                 **
 **  Website : https://github.com/heterophyllus/glassplotter                **
 **  Date    : 2020-1-25                                                    **
 *****************************************************************************/


#include "dndt_plot_form.h"
#include "ui_dndt_plot_form.h"

DnDtPlotForm::DnDtPlotForm(QList<GlassCatalog*> catalogList, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DnDtPlotForm)
{
    ui->setupUi(this);
    this->setWindowTitle("Dn/Dt(abs) Plot");

    m_catalogList = catalogList;

    m_customPlot = ui->widget;
    m_customPlot->setInteractions(QCP::iSelectAxes | QCP::iSelectLegend | QCP::iSelectPlottables);
    m_customPlot->xAxis->setLabel("Temperature(C)");
    m_customPlot->yAxis->setLabel("Dn/Dt(abs)  [10^(-6) /K]");
    m_customPlot->legend->setVisible(true);
    m_customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom); // Legend position

    QObject::connect(ui->pushButton_SetGlass,   SIGNAL(clicked()),     this, SLOT(setGlass()));
    QObject::connect(ui->pushButton_AddGraph,   SIGNAL(clicked()),     this, SLOT(addGraph()));
    QObject::connect(ui->pushButton_DeleteGraph,SIGNAL(clicked()),     this, SLOT(deleteGraph()));
    QObject::connect(ui->pushButton_SetAxis,    SIGNAL(clicked()),     this, SLOT(setAxis()));
    QObject::connect(ui->pushButton_Clear,      SIGNAL(clicked()),     this, SLOT(clearAll()));
    QObject::connect(ui->checkBox_Legend,       SIGNAL(toggled(bool)), this, SLOT(setLegendVisible()));

    m_table = ui->tableWidget;
    setDefault();
}

DnDtPlotForm::~DnDtPlotForm()
{
    m_currentGlass = nullptr;
    m_catalogList.clear();
    m_wvlList.clear();
    m_table->clear();
    m_table = nullptr;
    m_customPlot->clearPlottables();
    m_customPlot = nullptr;

    delete ui;
}

void DnDtPlotForm::setGlass()
{
    GlassSelectionDialog *dlg = new GlassSelectionDialog(m_catalogList, this);
    if(dlg->exec() == QDialog::Accepted)
    {
        int catalogIndex = dlg->getCatalogIndex();
        QString glassName = dlg->getGlassName();
        Glass* newGlass = m_catalogList[catalogIndex]->glass(glassName);

        if(!newGlass->hasThermalData){
            QMessageBox::information(this,tr("Error"), tr("This glass does not have thermal data."));
            newGlass = nullptr;
            return;
        }

        clearAll();
        m_currentGlass = newGlass;
        ui->label_GlassName->setText( m_currentGlass->name() + "_" + m_currentGlass->supplyer() );

        m_wvlList  = QList<double>() << 435.8 << 546.1 << 587.0 << 852.1 << 1060.0;
        updateAll();

        // rescale y axis
        m_customPlot->yAxis->rescale();
        m_customPlot->replot();

        ui->lineEdit_Xmin->setText(QString::number(m_customPlot->xAxis->range().lower));
        ui->lineEdit_Xmax->setText(QString::number(m_customPlot->xAxis->range().upper));
        ui->lineEdit_Ymin->setText(QString::number(m_customPlot->yAxis->range().lower));
        ui->lineEdit_Ymax->setText(QString::number(m_customPlot->yAxis->range().upper));
    }
}

void DnDtPlotForm::addGraph()
{
    if(!m_currentGlass){
        QMessageBox::information(this,tr("Error"), tr("Set glass from the button."));
        return;
    }

    bool ok;
    double wvl = QInputDialog::getDouble(
               this,
               tr("Input new wavelength"),
               tr("wavelength(nm):"),
               587.0,
               0.0,
               10000,
               2,
               &ok
           );
    if(!ok) return;

    m_wvlList.append(wvl);
    updateAll();

}

void DnDtPlotForm::deleteGraph()
{
    if(m_customPlot->selectedGraphs().size() > 0)
    {
        QCPGraph* selectedGraph = m_customPlot->selectedGraphs().first();
        QString graphName = selectedGraph->name();

        for(int i = 0;i < m_wvlList.size(); i++){
            if(abs(m_wvlList[i] - graphName.toDouble()) < 0.1 ){
                m_wvlList.removeAt(i);
                break;
            }
        }
        updateAll();
    }
}

void DnDtPlotForm::setDefault()
{
    QCPRange xrange = QCPRange(-100,140);
    QCPRange yrange = QCPRange(0,23);

    m_customPlot->xAxis->setRange(xrange);
    m_customPlot->yAxis->setRange(yrange);

    ui->lineEdit_Xmin->setText(QString::number(xrange.lower));
    ui->lineEdit_Xmax->setText(QString::number(xrange.upper));
    ui->lineEdit_Ymin->setText(QString::number(yrange.lower));
    ui->lineEdit_Ymax->setText(QString::number(yrange.upper));
}

void DnDtPlotForm::setAxis()
{
    QCPRange xrange, yrange;
    xrange.lower = ui->lineEdit_Xmin->text().toDouble();
    xrange.upper = ui->lineEdit_Xmax->text().toDouble();
    yrange.lower = ui->lineEdit_Ymin->text().toDouble();
    yrange.upper = ui->lineEdit_Ymax->text().toDouble();

    m_customPlot->xAxis->setRange(xrange);
    m_customPlot->yAxis->setRange(yrange);

    updateAll();
}

void DnDtPlotForm::updateAll()
{
    if(!m_currentGlass) return;

    m_customPlot->clearGraphs();
    m_table->clear();

    QVector<double> xdata = QCPUtil::getVectorFromRange(m_customPlot->xAxis->range());
    QVector<double> ydata;
    QCPGraph* graph;

    int i,j;
    int rowCount = xdata.size();
    int columnCount = m_wvlList.size() + 1; // temperature + wavelengths
    m_table->setRowCount(rowCount);
    m_table->setColumnCount(columnCount);

    QStringList header = QStringList() << "Temperature";
    QTableWidgetItem* item;

    // replot all graphs and recreate tables
    double currentWvl;
    for(i = 0; i < m_wvlList.size(); i++)
    {
        currentWvl = m_wvlList[i]; // unit:nm

        // graphs
        ydata = QCPUtil::scaleVector(m_currentGlass->dn_dt_abs(xdata, currentWvl/1000), pow(10,6)); //unit:micron
        graph = m_customPlot->addGraph();
        graph->setName(QString::number(currentWvl));
        graph->setData(xdata, ydata);
        QCPUtil::setColorToGraph(graph, QCPUtil::getColorFromIndex(i,m_maxGraphCount));
        graph->setVisible(true);

        // table
        header << QString::number(currentWvl) + "nm";
        for(j = 0; j< rowCount; j++){

            // temperature
            item = new QTableWidgetItem;
            item->setText(QString::number(xdata[j]) );
            m_table->setItem(j, 0, item);

            // dn/dt(abs)
            item = new QTableWidgetItem;
            item->setText( QString::number(ydata[j]) );
            m_table->setItem(j, i+1, item);
        }

    }
    m_customPlot->replot();
    m_table->setHorizontalHeaderLabels(header);
}

void DnDtPlotForm::clearAll()
{
    m_wvlList.clear();

    m_customPlot->clearGraphs();
    m_customPlot->clearItems();
    m_customPlot->replot();

    m_table->clear();
    m_table->update();
}

void DnDtPlotForm::setLegendVisible()
{
    m_customPlot->legend->setVisible(ui->checkBox_Legend->checkState());
    m_customPlot->replot();
}