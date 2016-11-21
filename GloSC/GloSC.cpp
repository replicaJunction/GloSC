/*
Copyright 2016 Peter Repukat - FlatspotSoftware

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include "GloSC.h"

GloSC::GloSC(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	updateEntryList();

}

void GloSC::updateEntryList()
{
	ui.lwInstances->clear();

	QDir dir(".\\");
	QStringList dirNames = dir.entryList(QDir::Dirs);

	for (auto &dirName : dirNames)
	{
		if (dirName != "." && dirName != ".." && dirName != "platforms")
			ui.lwInstances->addItem(dirName);
	}


}

void GloSC::writeIni(QString entryName)
{
	QSettings settings(entryName + "\\TargetConfig.ini", QSettings::IniFormat);

	settings.beginGroup("BaseConf");

	settings.setValue("bShowOverlay", 0 + ui.cbOverlay->isChecked());
	settings.setValue("bEnableControllers", 0 + ui.cbControllers->isChecked());

	settings.endGroup();


	settings.beginGroup("LaunchGame");

	settings.setValue("bLaunchGame", 0 + ui.cbLaunchGame->isChecked());
	if (ui.cbLaunchGame->isChecked())
	{
		settings.setValue("Path", ui.lePath->text());
		if (ui.lePath->text().contains("\\") || ui.lePath->text().contains("/"))
		{
			settings.setValue("Type", "Win32");
		} else {
			settings.setValue("Type", "UWP");
		}
	}

	settings.setValue("bCloseWhenDone", 0 + ui.cbCloseWhenDone->isChecked());

	settings.endGroup();

}


void GloSC::on_pbSave_clicked()
{
	QString name = ui.leName->text();
	name.remove("\\");
	name.remove("/");
	name.remove(":");
	name.remove(".");

	QString temp = name;
	if (temp.remove(" ") == "")
		return;

	QDir dir(name);
	if (!dir.exists())
		dir.mkdir(".");

#ifdef NDEBUG
	QString fileNames[] = {
		"Qt5Core.dll",
		"Qt5Gui.dll",
		"Qt5Widgets.dll",
		"sfml-system-2.dll",
		"sfml-window-2.dll",
		"sfml-graphics-2.dll",
		"ViGEmUM.dll",
		"TargetConfig.ini" };
#else
	QString fileNames[] = {
		"Qt5Cored.dll",
		"Qt5Guid.dll",
		"Qt5Widgetsd.dll",
		"sfml-system-d-2.dll",
		"sfml-window-d-2.dll",
		"sfml-graphics-d-2.dll",
		"ViGEmUM.dll",
		"TargetConfig.ini"
};
#endif

	for (auto &fileName : fileNames)
	{
		QFile::copy(fileName, dir.path() + "\\" + fileName);
	}
	QDir platformdir(name + "\\platforms");
	if (!platformdir.exists())
		platformdir.mkdir(".");


	QFile::copy("platforms\\qwindows.dll", dir.path() + "\\" + "platforms\\qwindows.dll");
	QFile::copy("SteamTarget.exe", dir.path() + "\\" + name + ".exe");

	writeIni(name);

	updateEntryList();

}


void GloSC::on_pbDelete_clicked()
{
	QString name = ui.leName->text();
	name.remove("\\");
	name.remove("/");
	name.remove(":");
	name.remove(".");

	QString temp = name;
	if (temp.remove(" ") == "")
		return;

	QDir dir(name);
	if (dir.exists())
	{
		dir.removeRecursively();
	}
	updateEntryList();
}

void GloSC::on_pbAddToSteam_clicked()
{
	QSettings settings("HKEY_CURRENT_USER\\SOFTWARE\\Valve\\Steam", QSettings::NativeFormat);
	QString steamPath = settings.value("SteamPath").toString();
	QString activeUser = settings.value("ActiveProcess/ActiveUser").toString();

	QFile shortcutsFile(steamPath + "/userdata/" + activeUser + "/config/shortcuts.vdf");

	if (!shortcutsFile.exists())
	{
		QMessageBox::information(this, "GloSC", "Couldn't detect Steam shortcuts file!", QMessageBox::Ok);
		return;
	}
	if (!shortcutsFile.open(QFile::ReadWrite))
	{
		QMessageBox::information(this, "GloSC", "Couldn't open Steam shortcuts file!", QMessageBox::Ok);
		return;
	}

	//just detect already present paths the easy way and hardcode the actual shortcut structure
	//will prob. come back to bite me, but for now it should be enough
	QByteArray shortcutsFileBytes = shortcutsFile.readAll();

	//get shortcutcount
	QByteArray temp = shortcutsFileBytes;
	temp.chop(9); //chop off last "tags"
	temp = temp.mid(temp.lastIndexOf("tags") + 8, temp.size() - 1);
	int shortcutCount = QString(temp).toInt();

	QString itemName;
	QString appDir = QDir::toNativeSeparators(QCoreApplication::applicationFilePath().mid(0, QCoreApplication::applicationFilePath().lastIndexOf("/")));
	for (int i = 0; i < ui.lwInstances->count(); i++)
	{
		itemName = ui.lwInstances->item(i)->text();
		if (!shortcutsFileBytes.contains(QString(appDir + "\\" + itemName + "\\" + itemName + ".exe").toStdString().c_str()))
		{
			shortcutsFileBytes.chop(2); //chop of end bytes
			shortcutCount++;

			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append(QString::number(shortcutCount));
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x01');
			shortcutsFileBytes.append("appname");
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append(itemName);
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x01');
			shortcutsFileBytes.append("exe");
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append(QString("\"" + appDir + "\\" + itemName + "\\" + itemName + ".exe\""));
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x01');
			shortcutsFileBytes.append("StartDir");
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append(QString("\"" + appDir + "\\" + itemName + "\\" + "\""));
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x01');
			shortcutsFileBytes.append("icon");
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x01');
			shortcutsFileBytes.append("ShortcutPath");
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x02');
			shortcutsFileBytes.append("IsHidden");
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x02');
			shortcutsFileBytes.append("AllowDesktopConfig");
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x01');
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x02');
			shortcutsFileBytes.append("OpenVR");
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append("tags");
			shortcutsFileBytes.append('\x00');
			shortcutsFileBytes.append('\x08');
			shortcutsFileBytes.append('\x08');

			//append chopped of bytes
			shortcutsFileBytes.append('\x08');
			shortcutsFileBytes.append('\x08');
		}
	}

	shortcutsFile.close();
	if (!shortcutsFile.open(QFile::ReadWrite | QIODevice::Truncate)) //damn qt is complicated...
	{
		QMessageBox::information(this, "GloSC", "Couldn't open Steam shortcuts file!", QMessageBox::Ok);
		return;
	}

	shortcutsFile.write(shortcutsFileBytes);

	shortcutsFile.close();
	QMessageBox::information(this, "GloSC", "Shortcuts were added! Restart Steam for changes to take effect!", QMessageBox::Ok);

}

void GloSC::on_pbSearchPath_clicked()
{
	QString filePath = QFileDialog::getOpenFileName(this, "Select Game", "", "*.exe");
	ui.lePath->setText(filePath);
}

void GloSC::on_pbUWP_clicked()
{
	QSettings *settings = new QSettings("HKEY_CLASSES_ROOT", QSettings::NativeFormat);

	QStringList childs = settings->childGroups();
	QStringList packages;

	for (auto& child : childs)
	{
		if (child.indexOf("AppX") == 0)
		{
			packages << child;
		}
	}

	delete settings;


	QList<UWPPair> pairs;

	QString AppName;
	QString AppUMId;

	QStringList AppNames;
	QStringList AppUMIds;


	for (auto &package : packages)
	{
		settings = new QSettings("HKEY_CLASSES_ROOT\\"+package, QSettings::NativeFormat);

		AppName = settings->value("Application/ApplicationName").toString();
		AppUMId = settings->value("Application/AppUserModelID").toString();
		if (!AppNames.contains(AppName) && !AppUMIds.contains(AppUMId) && AppUMId.size() > 0)
		{

			AppNames << AppName;
			AppUMIds << AppUMId;

			if (AppName.size() == 0)
			{
				AppName = "Unknown";
			} else if (AppName.at(0) == '@') {
				QString packageName = AppName.mid(AppName.indexOf('{') + 1, AppName.size() -1);
				packageName = packageName.mid(0, packageName.indexOf('?'));
				QStringList cachedNameChildGroups;
				QSettings settings("HKEY_CLASSES_ROOT\\Local Settings\\MrtCache", QSettings::NativeFormat);

				cachedNameChildGroups = settings.childGroups();

				for (auto &childGroup : cachedNameChildGroups)
				{
					
					if (childGroup.contains(packageName))
					{
						QSettings settings("HKEY_CLASSES_ROOT\\Local Settings\\MrtCache\\"+ childGroup, QSettings::NativeFormat);

						QStringList allKeys = settings.allKeys();

						AppName.replace("/", "\\");
						for (auto &key : allKeys)
						{
							if (key.contains(AppName))
							{
								AppName = settings.value(key).toString();
								break;
							}
						}

						break;
					}
				}
				if (AppName.at(0) == '@') {
					AppName = "Unknown";
				}
			}

			UWPPair uwpPair = {
				AppName,
				AppUMId,
			};

			pairs.push_back(uwpPair);

		}
		delete settings;
	}

	uwpPairs = pairs;


	UWPSelectDialog dialog(this);
	dialog.setUWPList(uwpPairs);
	int selection = dialog.exec();

	if (selection > -1)
	{
		ui.lePath->setText(uwpPairs.at(selection).AppUMId);
	}

}

void GloSC::on_lwInstances_currentRowChanged(int row)
{
	if (row < 0)
		return;
	QString entryName = ui.lwInstances->item(row)->text();
	ui.leName->setText(entryName);

	QSettings settings(entryName + "\\TargetConfig.ini", QSettings::IniFormat);

	settings.beginGroup("BaseConf");

	ui.cbOverlay->setChecked(settings.value("bShowOverlay").toBool());
	ui.cbControllers->setChecked(settings.value("bEnableControllers").toBool());

	settings.endGroup();


	settings.beginGroup("LaunchGame");

	ui.cbLaunchGame->setChecked(settings.value("bLaunchGame").toBool());
	if (ui.cbLaunchGame->isChecked())
	{
		ui.lePath->setText(settings.value("Path").toString());
	}
	ui.cbCloseWhenDone->setChecked(settings.value("bCloseWhenDone").toBool());

	settings.endGroup();


}