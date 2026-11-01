/****************************************************************************
**
** Copyright (C) 2004-2008 NIISI RAS. All rights reserved.
**
** This file is part of the KuMir.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/
#ifndef PULT_H
#define PULT_H

#include "ui_pult.h"
#include "kuznec.h"
#include <kumir2-libs/widgets/pultlogger.h>


class GrasshopperPult : public QWidget, public Ui::GrasshopperPult
{
	Q_OBJECT
public:
	/**
	 * Конструктор
	 * @param parent ссыка на объект-владелец
	 * @param fl флаги окна
	 */
	GrasshopperPult(
		const QDir &resDir, KumKuznec *kobj,
		QWidget *parent = 0,
		Qt::WindowFlags fl = Qt::WindowFlags()
	);
	~GrasshopperPult();

	bool Link()
	{
		return link;
	}

	pultLogger *pltLogger()
	{
		return Logger;
	}

	bool getLibMode() const
	{
		return libMode;
	}

	void AutoClose()
	{
		autoClose = true;
	}

public slots:
	void noLink();
	void LinkOK();

	void newClient(QString);
	void clientDisconnect();

	void Up();
	void Down();
	void Left();
	void Right();
	void ColorUnColor();
	void SwStena();
	void SwSvobodno();
	void TempS();
	void RadS();

	void resetKuznec();
	void showMessage(QString message)
	{
		label->setText(message);
	}

	void logToKumir();

	void setStepSizes(int Fwd, int Back)
	{
		FwdNumber->display(Fwd);
		BackNumber->display(Back);
	}

signals:
	void goUp();
	void goDown();
	void goLeft();
	void goRight();

	void hasLeftWall();
	void hasRightWall();

	void noUpWall();
	void noDownWall();
	void noLeftWall();
	void noRightWall();

	void Color();

	void Clean();

	void Rad();
	void Temp();
	void logToK();
	void PultCmd(QString text);
	void sendText(QString text);

protected:
	virtual void closeEvent(QCloseEvent *event);
	virtual void paintEvent(QPaintEvent *event);

private:
	KumKuznec *kuznecObj;
	pultLogger *Logger;
	bool link;
	bool libMode;
	bool autoClose;

	linkLight *greenLight;
	MainButton *buttFwd, *buttBack, *turnLeft, *turnRight, *recolorB;
	MainButton *askStena, *askFree, *buttRad, *buttTemp;

};
#endif
