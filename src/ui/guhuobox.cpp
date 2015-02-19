#include "guhuobox.h"
#include "button.h"
#include "engine.h"
#include "standard.h"
#include "clientplayer.h"
#include "skinbank.h"

GuhuoBox::GuhuoBox(const QString &skillname, const QString &flag, bool playonly){
    this->skill_name = skillname;
    this->flags = flag;
    this->play_only = playonly;
    title  = QString("%1 %2").arg(Sanguosha->translate(skill_name)).arg(tr("Please choose:"));;
    //collect Cards' objectNames
    if(flags.contains("b")){
        QList<const BasicCard*> basics = Sanguosha->findChildren<const BasicCard*>();
        foreach(const BasicCard *card,basics){
            if(!card_list["BasicCard"].contains(card->objectName())
                    && !ServerInfo.Extensions.contains("!" + card->getPackage()))
                card_list["BasicCard"].append(card->objectName());
        }
    }
    if(flags.contains("t")){
        QList<const TrickCard*> tricks = Sanguosha->findChildren<const TrickCard*>();
        foreach(const TrickCard *card,tricks){
            if(!ServerInfo.Extensions.contains("!" + card->getPackage()) && card->isNDTrick()){
                if(card_list["SingleTargetTrick"].contains(card->objectName())
                        || card_list["MultiTarget"].contains(card->objectName()))
                    continue;
                if(card->inherits("SingleTargetTrick") && !card_list["SingleTargetTrick"].contains(card->objectName()))
                    card_list["SingleTargetTrick"].append(card->objectName());
                else
                    card_list["MultiTarget"].append(card->objectName());

            }
        }
    }
    if(flags.contains("d")){
        QList<const DelayedTrick*> delays = Sanguosha->findChildren<const DelayedTrick*>();
        foreach(const DelayedTrick *card,delays){
            if(!card_list["DelayedTrick"].contains(card->objectName())
                    && !ServerInfo.Extensions.contains("!" + card->getPackage()))
                card_list["DelayedTrick"].append(card->objectName());
        }
    }
//    if(flags.contains("e")){
//        QList<const EquipCard*> equips = Sanguosha->findChildren<const EquipCard*>();
//        foreach(const EquipCard *card,equips){
//            if(!card_list["EquipCard"].contains(card->objectName())
//                    && !ServerInfo.Extensions.contains("!" + card->getPackage()))
//                card_list["EquipCard"].append(card->objectName());
//        }
//    }
}


QRectF GuhuoBox::boundingRect() const {
    const int width = getButtonWidth()*4 + outerBlankWidth * 5; // 4 buttons each line

    int height = topBlankWidth +
            ((card_list["BasicCard"].length()+3)/4) * defaultButtonHeight
            + (((card_list["BasicCard"].length()+3)/4) - 1) * interval
            +((card_list["SingleTargetTrick"].length()+3)/4) * defaultButtonHeight
            + (((card_list["SingleTargetTrick"].length()+3)/4) - 1) * interval
            +((card_list["MultiTarget"].length()+3)/4) * defaultButtonHeight
            + (((card_list["MultiTarget"].length()+3)/4) - 1) * interval
            +((card_list["DelayedTrick"].length()+3)/4) * defaultButtonHeight
            + (((card_list["DelayedTrick"].length()+3)/4) - 1) * interval
            +((card_list["EquipCard"].length()+3)/4) * defaultButtonHeight
            + (((card_list["EquipCard"].length()+3)/4) - 1) * interval
            +card_list.keys().length()*titleWidth*2 //add some titles......
            +defaultButtonHeight+interval //for cancel button
            + bottomBlankWidth;

    return QRectF(0, 0, width, height);
}
int GuhuoBox::getButtonWidth() const
{
    if (card_list.values().isEmpty())
        return minButtonWidth;

    QFontMetrics fontMetrics(Button::defaultFont());
    int biggest = 0;
    foreach (const QStringList &value, card_list.values()) {
        foreach(const QString choice,value){
            const int width = fontMetrics.width(translate(choice));
            if (width > biggest)
                biggest = width;
        }
    }

    // Otherwise it would look compact
    biggest += 20;

    int width = minButtonWidth;
    return qMax(biggest, width);
}

void GuhuoBox::popup(){
    if (play_only && Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY) {
        emit onButtonClick();
        return;
    }
    const int buttonWidth = getButtonWidth();
    foreach (const QString &key, card_list.keys()) {
        foreach(const QString &card_name,card_list.value(key)){
            Button *button = new Button(translate(card_name), QSizeF(buttonWidth,
                                                          defaultButtonHeight));
            button->setObjectName(card_name);
            buttons[card_name] = button;

            Card *ca = Sanguosha->cloneCard(card_name);
            button->setEnabled(ca->isAvailable(Self));
            ca->deleteLater();

            button->setParentItem(this);

            QString original_tooltip = QString(":%1").arg(title);
            QString tooltip = Sanguosha->translate(original_tooltip);
            if (tooltip == original_tooltip) {
                original_tooltip = QString(":%1").arg(card_name);
                tooltip = Sanguosha->translate(original_tooltip);
            }
            connect(button, &Button::clicked, this, &GuhuoBox::reply);
            if (tooltip != original_tooltip)
                button->setToolTip(QString("<font color=%1>%2</font>")
                                   .arg(Config.SkillDescriptionInToolTipColor.name())
                                   .arg(tooltip));
        }
        titles[key] = new Title(this,translate(key),Button::defaultFont().toString().split(" ").first(),Config.TinyFont.pixelSize()); //undefined reference to "GuhuoBox::titleWidth" 666666
        titles[key]->setParentItem(this);
    }
    moveToCenter();
    setZValue(1);
    show();
    int x = 0;
    int y = 0;
    int titles_num = 0;
    foreach(const QString &key, card_list.keys()){
        titles[key]->setPos(interval,
                            topBlankWidth +
                            defaultButtonHeight * y + (y - 1) * interval
                            + titleWidth*titles_num);
        ++titles_num;
        foreach(const QString &card_name,card_list.value(key)){
            QPointF apos;
            apos.setX((x+1)*outerBlankWidth + x*buttonWidth);
            apos.setY(topBlankWidth+
                      defaultButtonHeight*y+
                      interval*(y-1)+
                      titleWidth*titles_num*2
                      );
            ++x;
            if (x == 4){
                ++y;
                x = 0;
            }
            buttons[card_name]->setPos(apos);
        }
        ++y;
        x = 0;
    }
    cancel = new Button(translate("cancel"), QSizeF(buttonWidth,
                                               defaultButtonHeight));
    cancel->setParentItem(this);
    cancel->setObjectName("cancel");
    cancel->setPos(boundingRect().width()/2 - getButtonWidth()/2,boundingRect().height()-titleWidth*3);

    connect(cancel, &Button::clicked, this, &GuhuoBox::reply);

}
void GuhuoBox::reply(){
    if(!Self->tag[skill_name].isNull())
        Self->tag[skill_name] = QVariant();
    emit onButtonClick();
    const QString &answer = sender()->objectName();
    if(answer == "cancel" ){
        clear();
        return;
    }
    Self->tag[skill_name] = answer;
    clear();
}
void GuhuoBox::clear(){
    foreach (Button *button, buttons.values())
        button->deleteLater();

    buttons.values().clear();

    foreach(Title *title,titles.values()){
        title->deleteLater();
    }

    titles.values().clear();

    cancel->deleteLater();

    disappear();
}
QString GuhuoBox::translate(const QString &option) const
{
    QString title = QString("%1:%2").arg(skill_name).arg(option);
    QString translated = Sanguosha->translate(title);
    if (translated == title)
        translated = Sanguosha->translate(option);
    return translated;
}

//GuhuoBox *GuhuoBox::getInstance(const QString &skill_name, const QString &flags, bool play_only){
//    static GuhuoBox *instance;
//    if(instance == NULL || instance->getSkillName() != skill_name){
//        instance = new GuhuoBox(skill_name,flags,play_only);
//    }
//    return instance;
//}
