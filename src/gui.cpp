#include <QApplication>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QPrinter>
#include <QPainter>
#include <QKeyEvent>
#include <QSignalMapper>
#include "config.h"
#include "icons.h"
#include "keys.h"
#include "gpx.h"
#include "map.h"
#include "maplist.h"
#include "elevationgraph.h"
#include "speedgraph.h"
#include "track.h"
#include "infoitem.h"
#include "filebrowser.h"
#include "gui.h"


static QString timeSpan(qreal time)
{
	unsigned h, m, s;

	h = time / 3600;
	m = (time - (h * 3600)) / 60;
	s = time - (h * 3600) - (m * 60);

	return QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0'))
	  .arg(s,2, 10, QChar('0'));
}


GUI::GUI()
{
	loadFiles();

	createActions();
	createMenus();
	createToolBars();
	createTrackView();
	createTrackGraphs();
	createStatusBar();

	connect(_elevationGraph, SIGNAL(sliderPositionChanged(qreal)), _track,
	  SLOT(movePositionMarker(qreal)));
	connect(_speedGraph, SIGNAL(sliderPositionChanged(qreal)), _track,
	  SLOT(movePositionMarker(qreal)));

	_browser = new FileBrowser(this);
	_browser->setFilter(QStringList("*.gpx"));

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(_track);
	layout->addWidget(_trackGraphs);

	QWidget *widget = new QWidget;
	widget->setLayout(layout);
	setCentralWidget(widget);

	setWindowTitle(APP_NAME);
	setUnifiedTitleAndToolBarOnMac(true);

	_distance = 0;
	_time = 0;

	resize(600, 800);
}

void GUI::loadFiles()
{
	// Maps
	_maps = MapList::load(QString("%1/"MAP_LIST_FILE).arg(QDir::homePath()));

	// POI files
	QDir dir(QString("%1/"POI_DIR).arg(QDir::homePath()));
	QFileInfoList list = dir.entryInfoList(QStringList(), QDir::Files);
	for (int i = 0; i < list.size(); ++i) {
		if (!_poi.loadFile(list.at(i).absoluteFilePath()))
			fprintf(stderr, "Error loading POI file: %s: %s",
			  qPrintable(list.at(i).absoluteFilePath()),
			  qPrintable(_poi.errorString()));
	}
}

void GUI::createMapActions()
{
	QActionGroup *ag = new QActionGroup(this);
	ag->setExclusive(true);

	QSignalMapper *sm = new QSignalMapper(this);

	for (int i = 0; i < _maps.count(); i++) {
		QAction *a = new QAction(_maps.at(i)->name(), this);
		a->setCheckable(true);
		a->setActionGroup(ag);

		sm->setMapping(a, i);
		connect(a, SIGNAL(triggered()), sm, SLOT(map()));

		_mapActions.append(a);
	}

	connect(sm, SIGNAL(mapped(int)), this, SLOT(mapChanged(int)));

	_mapActions.at(0)->setChecked(true);
	_currentMap = _maps.at(0);
}

void GUI::createActions()
{
	// Action Groups
	_fileActionGroup = new QActionGroup(this);
	_fileActionGroup->setExclusive(false);
	_fileActionGroup->setEnabled(false);


	// General actions
	_exitAction = new QAction(QIcon(QPixmap(QUIT_ICON)), tr("Quit"), this);
	_exitAction->setShortcut(QKeySequence::Quit);
	connect(_exitAction, SIGNAL(triggered()), this, SLOT(close()));

	// Help & About
	_dataSourcesAction = new QAction(tr("Data sources"), this);
	connect(_dataSourcesAction, SIGNAL(triggered()), this, SLOT(dataSources()));
	_keysAction = new QAction(tr("Keyboard controls"), this);
	connect(_keysAction, SIGNAL(triggered()), this, SLOT(keys()));
	_aboutAction = new QAction(QIcon(QPixmap(APP_ICON)),
	  tr("About GPXSee"), this);
	connect(_aboutAction, SIGNAL(triggered()), this, SLOT(about()));
	_aboutQtAction = new QAction(QIcon(QPixmap(QT_ICON)), tr("About Qt"), this);
	connect(_aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

	// File related actions
	_openFileAction = new QAction(QIcon(QPixmap(OPEN_FILE_ICON)),
	  tr("Open"), this);
	_openFileAction->setShortcut(QKeySequence::Open);
	connect(_openFileAction, SIGNAL(triggered()), this, SLOT(openFile()));
	_saveFileAction = new QAction(QIcon(QPixmap(SAVE_FILE_ICON)),
	  tr("Save"), this);
	_saveFileAction->setShortcut(QKeySequence::Save);
	_saveFileAction->setActionGroup(_fileActionGroup);
	connect(_saveFileAction, SIGNAL(triggered()), this, SLOT(saveFile()));
	_saveAsAction = new QAction(QIcon(QPixmap(SAVE_AS_ICON)),
	  tr("Save as"), this);
	_saveAsAction->setShortcut(QKeySequence::SaveAs);
	_saveAsAction->setActionGroup(_fileActionGroup);
	connect(_saveAsAction, SIGNAL(triggered()), this, SLOT(saveAs()));
	_closeFileAction = new QAction(QIcon(QPixmap(CLOSE_FILE_ICON)),
	  tr("Close"), this);
	_closeFileAction->setShortcut(QKeySequence::Close);
	_closeFileAction->setActionGroup(_fileActionGroup);
	connect(_closeFileAction, SIGNAL(triggered()), this, SLOT(closeFile()));
	_reloadFileAction = new QAction(QIcon(QPixmap(RELOAD_FILE_ICON)),
	  tr("Reload"), this);
	_reloadFileAction->setShortcut(QKeySequence::Refresh);
	_reloadFileAction->setActionGroup(_fileActionGroup);
	connect(_reloadFileAction, SIGNAL(triggered()), this, SLOT(reloadFile()));

	// POI actions
	_openPOIAction = new QAction(QIcon(QPixmap(OPEN_FILE_ICON)),
	  tr("Load POI file"), this);
	connect(_openPOIAction, SIGNAL(triggered()), this, SLOT(openPOIFile()));
	_showPOIAction = new QAction(QIcon(QPixmap(SHOW_POI_ICON)),
	  tr("Show POIs"), this);
	_showPOIAction->setCheckable(true);
	connect(_showPOIAction, SIGNAL(triggered(bool)), this, SLOT(showPOI(bool)));

	// Map actions
	_showMapAction = new QAction(QIcon(QPixmap(SHOW_MAP_ICON)), tr("Show map"),
	  this);
	_showMapAction->setCheckable(true);
	connect(_showMapAction, SIGNAL(triggered(bool)), this, SLOT(showMap(bool)));
	if (_maps.empty())
		_showMapAction->setEnabled(false);
	else
		createMapActions();

	// Settings actions
	_showGraphsAction = new QAction(tr("Show graphs"), this);
	_showGraphsAction->setCheckable(true);
	_showGraphsAction->setChecked(true);
	connect(_showGraphsAction, SIGNAL(triggered(bool)), this,
	  SLOT(showGraphs(bool)));
	_showToolbarsAction = new QAction(tr("Show toolbars"), this);
	_showToolbarsAction->setCheckable(true);
	_showToolbarsAction->setChecked(true);
	connect(_showToolbarsAction, SIGNAL(triggered(bool)), this,
	  SLOT(showToolbars(bool)));
}

void GUI::createMenus()
{
	_fileMenu = menuBar()->addMenu(tr("File"));
	_fileMenu->addAction(_openFileAction);
	_fileMenu->addSeparator();
	_fileMenu->addAction(_saveFileAction);
	_fileMenu->addAction(_saveAsAction);
	_fileMenu->addSeparator();
	_fileMenu->addAction(_reloadFileAction);
	_fileMenu->addSeparator();
	_fileMenu->addAction(_closeFileAction);
#ifndef Q_OS_MAC
	_fileMenu->addSeparator();
	_fileMenu->addAction(_exitAction);
#endif // Q_OS_MAC

	_mapMenu = menuBar()->addMenu(tr("Map"));
	_mapMenu->addActions(_mapActions);
	_mapMenu->addSeparator();
	_mapMenu->addAction(_showMapAction);

	_poiMenu = menuBar()->addMenu(tr("POI"));
	_poiMenu->addAction(_openPOIAction);
	_poiMenu->addAction(_showPOIAction);

	_settingsMenu = menuBar()->addMenu(tr("Settings"));
	_settingsMenu->addAction(_showToolbarsAction);
	_settingsMenu->addAction(_showGraphsAction);

	_helpMenu = menuBar()->addMenu(tr("Help"));
	_helpMenu->addAction(_dataSourcesAction);
	_helpMenu->addAction(_keysAction);
	_helpMenu->addSeparator();
	_helpMenu->addAction(_aboutAction);
	_helpMenu->addAction(_aboutQtAction);
}

void GUI::createToolBars()
{
	_fileToolBar = addToolBar(tr("File"));
	_fileToolBar->addAction(_openFileAction);
	_fileToolBar->addAction(_saveFileAction);
	_fileToolBar->addAction(_reloadFileAction);
	_fileToolBar->addAction(_closeFileAction);
#ifdef Q_OS_MAC
	_fileToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
#endif // Q_OS_MAC

	_showToolBar = addToolBar(tr("Show"));
	_showToolBar->addAction(_showPOIAction);
	_showToolBar->addAction(_showMapAction);
#ifdef Q_OS_MAC
	_showToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
#endif // Q_OS_MAC
}

void GUI::createTrackView()
{
	_track = new Track(this);
}

void GUI::createTrackGraphs()
{
	_elevationGraph = new ElevationGraph;
	_speedGraph = new SpeedGraph;

	_trackGraphs = new QTabWidget;
	_trackGraphs->addTab(_elevationGraph, tr("Elevation"));
	_trackGraphs->addTab(_speedGraph, tr("Speed"));
	connect(_trackGraphs, SIGNAL(currentChanged(int)), this,
	  SLOT(graphChanged(int)));

	_trackGraphs->setFixedHeight(200);
	_trackGraphs->setSizePolicy(
		QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed));
}

void GUI::createStatusBar()
{
	_fileNameLabel = new QLabel();
	_distanceLabel = new QLabel();
	_timeLabel = new QLabel();
	_distanceLabel->setAlignment(Qt::AlignHCenter);
	_timeLabel->setAlignment(Qt::AlignHCenter);

	statusBar()->addPermanentWidget(_fileNameLabel, 8);
	statusBar()->addPermanentWidget(_distanceLabel, 1);
	statusBar()->addPermanentWidget(_timeLabel, 1);
	statusBar()->setSizeGripEnabled(false);
}

void GUI::about()
{
	QMessageBox msgBox(this);

	msgBox.setWindowTitle(tr("About GPXSee"));
	msgBox.setText(QString("<h3>") + QString(APP_NAME" "APP_VERSION)
	  + QString("</h3><p>") + tr("GPX viewer and analyzer") + QString("<p/>"));
	msgBox.setInformativeText(QString("<table width=\"300\"><tr><td>")
	  + tr("GPXSee is distributed under the terms of the GNU General Public "
	  "License version 3. For more info about GPXSee visit the project "
	  "homepage at ") + QString("<a href=\""APP_HOMEPAGE"\">"APP_HOMEPAGE
	  "</a>.</td></tr></table>"));

	QIcon icon = msgBox.windowIcon();
	QSize size = icon.actualSize(QSize(64, 64));
	msgBox.setIconPixmap(icon.pixmap(size));

	msgBox.exec();
}

void GUI::keys()
{
	QMessageBox msgBox(this);

	msgBox.setWindowTitle(tr("Keyboard controls"));
	msgBox.setText(QString("<h3>") + tr("Keyboard controls") + QString("</h3>"));
	msgBox.setInformativeText(
	  QString("<div><table width=\"300\"><tr><td>") + tr("Next file")
	  + QString("</td><td><i>SPACE</i></td></tr><tr><td>") + tr("Previous file")
	  + QString("</td><td><i>BACKSPACE</i></td></tr><tr><td>")
	  + tr("Append modifier") + QString("</td><td><i>SHIFT</i></td></tr>"
	  "</table></div>"));

	msgBox.exec();
}

void GUI::dataSources()
{
	QMessageBox msgBox(this);

	msgBox.setWindowTitle(tr("Data sources"));
	msgBox.setText(QString("<h3>") + tr("Data sources") + QString("</h3>"));
	msgBox.setInformativeText(
	  QString("<h4>") + tr("Map sources") + QString("</h4><p>")
	  + tr("Map (tiles) source URLs are read on program startup from the "
		"following file:")
		+ QString("</p><p><code>") + QDir::homePath()
		  + QString("/"MAP_LIST_FILE"</code></p><p>")
		+ tr("The file format is one map entry per line, consisting of the map "
		  "name and tiles URL delimited by a TAB character. The tile X and Y "
		  "coordinates are replaced with $x and $y in the URL and the zoom "
		  "level is replaced with $z. An example map file could look like:")
		+ QString("</p><p><code>Map1	http://tile.server.com/map/$z/$x/$y.png"
		  "<br/>Map2	http://mapserver.org/map/$z-$x-$y</code></p>")

	  + QString("<h4>") + tr("POIs") + QString("</h4><p>")
	  + tr("To make GPXSee load a POI file automatically on startup, add "
		"the file to the following directory:")
		+ QString("</p><p><code>") + QDir::homePath()
		+ QString("/"POI_DIR"</code></p>")
	);

	msgBox.exec();
}

void GUI::openFile()
{
	QStringList files = QFileDialog::getOpenFileNames(this, tr("Open file"));
	QStringList list = files;

	for (QStringList::Iterator it = list.begin(); it != list.end(); it++)
		openFile(*it);
}

bool GUI::openFile(const QString &fileName)
{
	if (fileName.isEmpty() || _files.contains(fileName))
		return false;

	if (loadFile(fileName)) {
		_files.append(fileName);
		_browser->setCurrent(fileName);
		updateStatusBarInfo();
		_fileActionGroup->setEnabled(true);
		return true;
	} else
		return false;
}

bool GUI::loadFile(const QString &fileName)
{
	GPX gpx;

	if (gpx.loadFile(fileName)) {
		_elevationGraph->loadGPX(gpx);
		_speedGraph->loadGPX(gpx);
		_track->loadGPX(gpx);
		if (_showPOIAction->isChecked())
			_track->loadPOI(_poi);

		_distance += gpx.distance();
		_time += gpx.time();

		return true;
	} else {
		QString error = fileName + QString("\n\n")
		  + tr("Error loading GPX file:\n%1").arg(gpx.errorString())
		  + QString("\n");
		if (gpx.errorLine())
			error.append(tr("Line: %1").arg(gpx.errorLine()));

		QMessageBox::critical(this, tr("Error"), error);
		return false;
	}
}

void GUI::openPOIFile()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open POI file"));

	if (!fileName.isEmpty()) {
		if (!_poi.loadFile(fileName)) {
			QString error = tr("Error loading POI file:\n%1")
			  .arg(_poi.errorString()) + QString("\n");
			if (_poi.errorLine())
				error.append(tr("Line: %1").arg(_poi.errorLine()));
			QMessageBox::critical(this, tr("Error"), error);
		} else {
			_showPOIAction->setChecked(true);
			_track->loadPOI(_poi);
		}
	}
}

void GUI::saveAs()
{
	QString fileName = QFileDialog::getSaveFileName(this, "Export to PDF",
	  QString(), "*.pdf");

	if (!fileName.isEmpty()) {
		saveFile(fileName);
		_saveFileName = fileName;
	}
}

void GUI::saveFile()
{
	if (_saveFileName.isEmpty())
		emit saveAs();
	else
		saveFile(_saveFileName);
}

void GUI::saveFile(const QString &fileName)
{
	QPrinter printer(QPrinter::HighResolution);
	printer.setPageSize(QPrinter::A4);
	printer.setOrientation(_track->orientation());
	printer.setOutputFormat(QPrinter::PdfFormat);
	printer.setOutputFileName(fileName);

	QPainter p(&printer);

	_track->plot(&p, QRectF(0, 300, printer.width(), (0.80 * printer.height())
	  - 400));
	_elevationGraph->plot(&p,  QRectF(0, 0.80 * printer.height(),
	  printer.width(), printer.height() * 0.20));

	QGraphicsScene scene;
	InfoItem info;
	info.insert(tr("Distance"), QString::number(_distance / 1000, 'f', 1)
	  + THIN_SPACE + tr("km"));
	info.insert(tr("Time"), timeSpan(_time));
	info.insert(tr("Ascent"), QString::number(_elevationGraph->ascent(), 'f', 0)
	  + THIN_SPACE + tr("m"));
	info.insert(tr("Descent"), QString::number(_elevationGraph->descent(), 'f',
	  0) + THIN_SPACE + tr("m"));
	info.insert(tr("Maximum"), QString::number(_elevationGraph->max(), 'f', 0)
	  + THIN_SPACE + tr("m"));
	info.insert(tr("Minimum"), QString::number(_elevationGraph->min(), 'f', 0)
	  + THIN_SPACE + tr("m"));
	scene.addItem(&info);
	scene.render(&p, QRectF(0, 0, printer.width(), 200));

	p.end();
}

void GUI::reloadFile()
{
	_distance = 0;
	_time = 0;

	_elevationGraph->clear();
	_speedGraph->clear();
	_track->clear();

	for (int i = 0; i < _files.size(); i++) {
		if (!loadFile(_files.at(i))) {
			_files.removeAt(i);
			i--;
		}
	}

	updateStatusBarInfo();
	if (_files.isEmpty())
		_fileActionGroup->setEnabled(false);
	else
		_browser->setCurrent(_files.last());
}

void GUI::closeFile()
{
	_distance = 0;
	_time = 0;

	_elevationGraph->clear();
	_speedGraph->clear();
	_track->clear();

	_files.clear();

	_fileActionGroup->setEnabled(false);
	updateStatusBarInfo();
}

void GUI::showPOI(bool checked)
{
	if (checked)
		_track->loadPOI(_poi);
	else
		_track->clearPOI();
}

void GUI::showMap(bool checked)
{
	if (checked)
		_track->setMap(_currentMap);
	else
		_track->setMap(0);
}

void GUI::showGraphs(bool checked)
{
	_trackGraphs->setHidden(!checked);
}

void GUI::showToolbars(bool checked)
{
	if (checked) {
		addToolBar(_fileToolBar);
		addToolBar(_showToolBar);
		_fileToolBar->show();
		_showToolBar->show();
	} else {
		removeToolBar(_fileToolBar);
		removeToolBar(_showToolBar);
	}
}

void GUI::updateStatusBarInfo()
{
	int files = _files.size();

	if (files == 0) {
		_fileNameLabel->clear();
		_distanceLabel->clear();
		_timeLabel->clear();
		return;
	} else if (files == 1)
		_fileNameLabel->setText(_files.at(0));
	else
		_fileNameLabel->setText(tr("%1 tracks").arg(_files.size()));

	_distanceLabel->setText(QString::number(_distance / 1000, 'f', 1)
	  + " " + tr("km"));
	_timeLabel->setText(timeSpan(_time));
}

void GUI::mapChanged(int index)
{
	_currentMap = _maps.at(index);

	if (_showMapAction->isChecked())
		_track->setMap(_currentMap);
}

void GUI::graphChanged(int index)
{
	if (_trackGraphs->widget(index) == _elevationGraph)
		_elevationGraph->setSliderPosition(_speedGraph->sliderPosition());
	else if (_trackGraphs->widget(index) == _speedGraph)
		_speedGraph->setSliderPosition(_elevationGraph->sliderPosition());
}

void GUI::keyPressEvent(QKeyEvent *event)
{
	QString file;

	if (event->key() == PREV_KEY)
		file = _browser->prev();
	if (event->key() == NEXT_KEY)
		file = _browser->next();

	if (!file.isNull()) {
		if (!(event->modifiers() & MODIFIER))
			closeFile();
		openFile(file);
	}
}
