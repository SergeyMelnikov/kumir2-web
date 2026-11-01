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
#ifndef NDIALOG_H
#define NDIALOG_H

#include "ui_dialog.h"

class Dialog : public QDialog, Ui::Dialog
{
	Q_OBJECT

public:
	Dialog(QWidget *parent = 0, Qt::WindowFlags fl = Qt::WindowFlags());
	~Dialog() {};

	int ASize() const
	{
		return BaseA->value();
	}

	int BSize() const
	{
		return BaseB->value();
	}

	int CSize() const
	{
		return BaseC->value();
	}

	int AFill() const
	{
		return ValueA->value();
	}

	int BFill() const
	{
		return ValueB->value();
	}

	int CFill() const
	{
		return ValueC->value();
	}

	int getNeed() const
	{
		return NeedA->value();
	}

	void setSizes(int a, int b, int c)
	{
		BaseA->setValue(a);
		BaseB->setValue(b);
		BaseC->setValue(c);
	}

	void setFill(int a, int b, int c)
	{
		ValueA->setValue(a);
		ValueB->setValue(b);
		ValueC->setValue(c);
	}

	void setNeed(int a)
	{
		NeedA->setValue(a);
	}

public slots:
	void setMax();

private:
	void translateButtons();
};

#endif // NDIALOG_H
