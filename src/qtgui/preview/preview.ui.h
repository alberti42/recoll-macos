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

void Preview::init()
{
    pvEdit->installEventFilter(this);
    searchTextLine->installEventFilter(this);
    dynSearchActive = false;
    canBeep = true;
    matchPara = 0;
    matchIndex = 0;
}

bool Preview::eventFilter(QObject *target, QEvent *event)
{

    if (event->type() != QEvent::KeyPress) 
	return QWidget::eventFilter(target, event);
    
    QKeyEvent *keyEvent = (QKeyEvent *)event;
    if (dynSearchActive) {
	if (keyEvent->key() == Key_F3) {
	    doSearch(true, false);
	    return true;
	}
	if (target != searchTextLine)
	    return QApplication::sendEvent(searchTextLine, event);
    } else {
	if (keyEvent->key() == Key_Slash && target == pvEdit) {
	    dynSearchActive = true;
	    return true;
	}
    }

    return QWidget::eventFilter(target, event);
}

void Preview::searchTextLine_textChanged(const QString & text)
{
    //fprintf(stderr, "search line text changed. text: '%s'\n", text.ascii());
    if (text.isEmpty()) {
	dynSearchActive = false;
    } else {
	dynSearchActive = true;
	doSearch(false, false);
    }
}


// Perform text search. If next is true, we look for the next match of the
// current search, trying to advance and possibly wrapping around. If next is
// false, the search string has been modified, we search for the new string, 
// starting from the current position
void Preview::doSearch(bool next, bool reverse)
{
    //fprintf(stderr, "Preview::doSearch: next %d rev %d para %d index %d\n",
    // int(next), int(reverse), matchPara, matchIndex);

    bool matchCase = matchCheck->isChecked();

    if (next) {
	// We search again, starting from the current match
	if (reverse) {
	    // when searching backwards, have to move back one char
	    if (matchIndex > 0)
		matchIndex --;
	    else if (matchPara > 0) {
		matchPara --;
		matchIndex = pvEdit->paragraphLength(matchPara);
	    }
	} else {
	    // Forward search: start from end of selection
	    int bogus;
	    pvEdit->getSelection(&bogus, &bogus, &matchPara, &matchIndex);
	    //fprintf(stderr, "New para: %d index %d\n",matchPara, matchIndex);
	}
    }

    bool found = pvEdit->find(searchTextLine->text(), matchCase, false, 
			      !reverse, &matchPara, &matchIndex);

    if (!found && next && true) { // need a 'canwrap' test here
	if (reverse) {
	    matchPara = pvEdit->paragraphs();
	    matchIndex = pvEdit->paragraphLength(matchPara);
	} else {
	    matchPara = matchIndex = 0;
	}
	found = pvEdit->find(searchTextLine->text(), matchCase, false, 
			     !reverse, &matchPara, &matchIndex);
    }

    if (found) {
	canBeep = true;
    } else {
	if (canBeep)
	    QApplication::beep();
	canBeep = false;
    }
}


void Preview::nextPressed()
{
    doSearch(true, false);
}


void Preview::prevPressed()
{
    doSearch(true, true);
}
