#include "dCacheTools.h"

#include <QLocale>
#include <QDir>
#include <QDateTime>

#include <boost/filesystem.hpp>

dCacheTools::dCacheTools()
{
	m_copy = false;
	m_check = false;
	m_list = false;

	m_dir = "";
	m_base = "";
	m_output = "";
	m_type = -1;
	m_isSingleFile = false;
	m_stop = false;
}

dCacheTools::~dCacheTools()
{
	dCacheCopy->deleteLater();
}

void dCacheTools::Configure(QString Input, QString BaseDir, QString OutputDir, int type, bool isSingleFile)
{
	m_dir = Input;
	m_base = BaseDir;
	m_output = OutputDir;
	m_type = type;
	m_isSingleFile = isSingleFile;
}

void dCacheTools::start()
{
	this->run();
}

void dCacheTools::run()
{
	if(m_copy)
		this->Copy();
	if(m_list)
		this->List();
	if(m_check)
		this->Check();

	return;
}

void dCacheTools::List()
{
	emit log("INFO", "Listing called");

	dCacheCopy = new QProcess();
	dCacheCopy->setProcessChannelMode(QProcess::ForwardedChannels);

	std::string str = "/usr/bin/gfal-ls -l ";
	str += "srm://dcache-se-desy.desy.de/pnfs/desy.de/calice/";
	str += (m_base+m_output).toStdString();

	dCacheCopy->start(QString::fromStdString(str));

	if(!dCacheCopy->waitForStarted())
	{
		emit log("ERROR", QString("gfal-ls %1").arg(dCacheCopy->errorString()));
		return;
	}

	dCacheCopy->waitForFinished();

	if(dCacheCopy->exitCode() != 0)
	{
		emit log("ERROR", dCacheCopy->errorString());
	}

	m_list = false;
}

void dCacheTools::Copy()
{
	emit log("MESSAGE", "gfal-cp");

	QLocale::setDefault(QLocale::English);

	if(m_isSingleFile)
	{
		nfiles = 1;
		idxProcess = 0;

		boost::filesystem::path p(m_dir.toStdString());
		std::string filename = p.filename().string();

		std::string str = "/usr/bin/gfal-copy --dry-run -n 5 -t 6000 ";
		str += "file:/";
		str += m_dir.toStdString();
		str += " srm://dcache-se-desy.desy.de/pnfs/desy.de/calice/";
		str += m_base.toStdString();
		str += "/";
		str += m_output.toStdString();
		str += filename;
		/*
        str += " lfn:/grid/calice/";
        str += BaseDir.toStdString();
        str += "/";
        str += OutputDir.toStdString();
        str += filename;
		 */

		if(m_stop)
		{
			this->StopCopy();
			return;
		}

		emit log("DEBUG", QString::fromStdString(str));

		dCacheCopy = new QProcess();
		dCacheCopy->setProcessChannelMode(QProcess::ForwardedChannels);
		dCacheCopy->start(QString::fromStdString(str));

		if(!dCacheCopy->waitForStarted())
		{
			emit log("ERROR", QString("gfal-copy %1").arg(dCacheCopy->errorString()));
			return;
		}

		connect(dCacheCopy, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(finishedProcess(int, QProcess::ExitStatus)));
	}
	else
	{
		QDir dir(m_dir);
		dir.setFilter(QDir::Files | QDir::NoSymLinks);
		dir.setSorting(QDir::Time | QDir::Reversed);

		list = dir.entryInfoList();
		nfiles = list.size();

		if(nfiles > 0)
		{
			QString filetype;
			if(m_type == 1)
				filetype = "txt";
			if(m_type == 2)
				filetype = "slcio";
			if(m_type == 3)
				filetype = "raw";

			idxProcess = 0;

			QFileInfo fileInfo = list.at(idxProcess);
			emit log("DEBUG", QString("file : %1 \t Time : %2").arg(fileInfo.fileName(), (fileInfo.lastModified()).toString(Qt::ISODate)));

			std::string filename = (fileInfo.fileName()).toStdString();
			if(fileInfo.completeSuffix() != filetype && m_type != 4)
				m_stop = true;

			if(m_stop)
			{
				this->StopCopy();
				return;
			}

			std::string str = "/usr/bin/gfal-copy --dry-run -n 5 -t 6000 ";
			str += "file:/";
			str += m_dir.toStdString();
			str += "/";
			str += filename;
			str += " srm://dcache-se-desy.desy.de/pnfs/desy.de/calice/";
			str += m_base.toStdString();
			str += "/";
			str += m_output.toStdString();
			str += filename;
			/*
            str += " lfn:/grid/calice/";
            str += BaseDir.toStdString();
            str += "/";
            str += OutputDir.toStdString();
            str += filename;
			 */

			emit log("DEBUG", QString::fromStdString(str));

			dCacheCopy = new QProcess();
			dCacheCopy->setProcessChannelMode(QProcess::ForwardedChannels);
			dCacheCopy->start(QString::fromStdString(str));

			if(!dCacheCopy->waitForStarted())
			{
				emit log("ERROR", QString("gfal-copy %1").arg(dCacheCopy->errorString()));
				//return;
			}

			connect(dCacheCopy, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(finishedProcess(int, QProcess::ExitStatus)));
		}
	}

	m_copy = false;
}

void dCacheTools::StopCopy()
{
	if(dCacheCopy->state() == QProcess::Running)
	{
		emit log("INFO", "Waiting for last Copy");
		dCacheCopy->waitForFinished();

		if(dCacheCopy->exitCode() != 0)
		{
			emit log("ERROR", dCacheCopy->errorString());
		}

		m_stop = false;

		dCacheCopy->deleteLater();
	}
}

void dCacheTools::Check()
{
	m_check = false;
}

void dCacheTools::finishedProcess (int exitCode, QProcess::ExitStatus exitStatus)
{
	emit log("INFO", "Complete");

	if(idxProcess < nfiles)
	{
		QString filetype;
		if(m_type == 1)
			filetype = "txt";
		if(m_type == 2)
			filetype = "slcio";
		if(m_type == 3)
			filetype = "raw";

		/* create QProcess object */
		dCacheCopy = new QProcess();
		dCacheCopy->setProcessChannelMode(QProcess::ForwardedChannels);

		QFileInfo fileInfo = list.at(idxProcess);
		emit log("DEBUG", QString("file : %1 \t Time : %2").arg(fileInfo.fileName(), (fileInfo.lastModified()).toString(Qt::ISODate)));

		std::string filename = (fileInfo.fileName()).toStdString();
		if(fileInfo.completeSuffix() != filetype && m_type != 4) m_stop = true;

		std::string str = "/usr/bin/gfal-copy --dry-run -n 5 -t 6000 ";
		str += "file:/";
		str += m_dir.toStdString();
		str += "/";
		str += filename;
		str += " srm://dcache-se-desy.desy.de/pnfs/desy.de/calice/";
		str += m_base.toStdString();
		str += "/";
		str += m_output.toStdString();
		str += filename;
		/*
    	            str += " lfn:/grid/calice/";
    	            str += BaseDir.toStdString();
    	            str += "/";
    	            str += OutputDir.toStdString();
    	            str += filename;
		 */

		if(m_stop)
		{
			this->StopCopy();
			return;
		}

		emit log("DEBUG", QString::fromStdString(str));

		dCacheCopy->start(QString::fromStdString(str));

		/* show output */
		if(!dCacheCopy->waitForStarted())
		{
			emit log("ERROR", QString("gfal-copy %1").arg(dCacheCopy->errorString()));
			//return;
		}

		connect(dCacheCopy, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(finishedProcess(int, QProcess::ExitStatus)));

		idxProcess++;
	}
}
