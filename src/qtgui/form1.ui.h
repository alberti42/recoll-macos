/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/

void Form1::fileExit()
{
    exit(0);
}


void Form1::helpIndex()
{

}


void Form1::helpContents()
{

}


void Form1::helpAbout()
{

}


void Form1::resTextEdit_clicked( int par, int car )
{
    fprintf(stderr, "Clicked at paragraph %d, char %d\n", par, car);
}
#include "qfontdialog.h"

#define BS 200000     
void Form1::resTextEdit_returnPressed()
{
    fprintf(stderr, "ReturnPressed()\n");
    resTextEdit->setFont( QFontDialog::getFont( 0, resTextEdit->font() ) );
    const char *fname = "utf8.txt";
    FILE *fp = fopen(fname, "r"); 
    if (fp) {
 char buf[BS];
                memset(buf,0, sizeof(buf));
 int n = fread(buf, 1, BS-1, fp);
                fclose(fp);
               QString str = QString::fromUtf8(buf, n);
               resTextEdit->setTextFormat(RichText);
               resTextEdit->setText(str);
    }

}
