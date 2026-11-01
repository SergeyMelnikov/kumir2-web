//****************************************************************************
//**
//** Copyright (C) 2004-2008 IMPB RAS. All rights reserved.
//**
//** This file is part of the Vodoley.
//**
//** This file may be used under the terms of the GNU General Public
//** License version 2.0 as published by the Free Software Foundation
//** and appearing in the file LICENSE.GPL included in the packaging of
//** this file.
//**
//** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
//** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//**
//****************************************************************************/


#include "ui_dialog.h"

class Dialog : public QDialog, Ui::Dialog
{
	Q_OBJECT

public:
	Dialog(QWidget *parent = 0, Qt::WindowFlags fl = Qt::WindowFlags());
	~Dialog();

	int ASize() const;
	int BSize() const;
	int ANeed() const;

	void setSizes(int A, int B);

	int getStart() const;

	void setStart(int pos);

	int getLeftBorder() const;
	int getRightBorder() const;

	void setLeftBorder(int pos);
	void setRightBorder(int pos);


	QList<int> getFlags();

	void setFlags(QList<int> flags);

	bool borders() const;

	void setBordersChB(bool brd);

	bool isTaskNeeded() const;

	void setTaskNeeded(bool task);

	void translateButtons();

public slots:
	void setMax();
	void addFlag();
	void enableDelete();
	void removeFlag();
};

