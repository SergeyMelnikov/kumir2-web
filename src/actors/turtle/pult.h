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

#ifndef TURTLEPULT_H
#define TURTLEPULT_H

#include <QWidget>
#include <QDir>

#include "ui_pult.h"

class pultLogger;
class linkLight;
class MainButton;


#define TEXT_STEP 14

namespace ActorTurtle
{
class TurtleModule;
}

class OvenTimer : public QWidget
{
	Q_OBJECT

public:
	OvenTimer(QWidget *parent = 0);

	void setDuration(int secs);
	int duration() const;
	void draw(QPainter *painter);

signals:
	void angChange(int value);

public slots:
	void setValue(int value);
protected:
	void paintEvent(QPaintEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
private:
	//  QDateTime finishTime;
	//  QTimer *updateTimer;
	//  QTimer *finishTimer;
	bool mouseFlag;
	QPointF old_mouse_pos;
	int gradValue;
	int oldValue;
};


class TurtlePult : public QWidget, public Ui::TurtlePult
{
	Q_OBJECT
public:
	/**
	     * Конструктор
	     * @param parent ссыка на объект-владелец
	     * @param fl флаги окна
	     */
	TurtlePult(QDir resDir = QDir(), QWidget *parent = 0, Qt::WindowFlags fl = Qt::WindowFlags());
	/**
	     * Деструктор
	     */
	~TurtlePult() {};

	bool Link()
	{
		return link;
	}

	pultLogger *Logger;

	pultLogger *pltLogger()
	{
		return Logger;
	}

	void connectTurtle();
	void AutoClose()
	{
		autoClose = true;
	}

public:
	ActorTurtle::TurtleModule *turtleObj;
	bool libMode;

	public
slots:
	void noLink();
	void LinkOK();

	void newClient(QString);
	void clientDisconnect();

	void Up();
	void Down();
	void Left();
	void Right();

	void SwStena();
	void SwSvobodno();
	void TempS();
	void RadS();

	void resetTurtle();
	void showMessage(QString message)
	{
		label->setText(message);
	}
	void logToKumir();

signals:
	void goUp(qreal step);
	void goDown();
	void goLeft();
	void goRight();

	void Color();

	void Clean();

	void logToK();
	void PultCmd(QString text);
	void sendText(QString);

protected:
	virtual void closeEvent(QCloseEvent *event);
	void paintEvent(QPaintEvent *event);

private:
	bool link;
	bool autoClose;
	linkLight *greenLight;
	MainButton *buttFwd, *buttBack, *turnLeft, *turnRight;
	MainButton *askStena, *askFree, *buttRad, *buttTemp;
	OvenTimer *GradInput;

};

#endif
