/*****************************************************************************
 * open.cpp : Panels for the open dialogs
 ****************************************************************************
 * Copyright (C) 2006-2009 the VideoLAN team
 * Copyright (C) 2007 Société des arts technologiques
 * Copyright (C) 2007 Savoir-faire Linux
 *
 * $Id: 8c3b50b0e9e589c32baadf4661628c86d5fead17 $
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Jean-Baptiste Kempf <jb@videolan.org>
 *          Pierre-Luc Beaudoin <pierre-luc.beaudoin@savoirfairelinux.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "qt4.hpp"
#include "components/open_panels.hpp"
#include "dialogs/open.hpp"
#include "dialogs_provider.hpp" /* Open Subtitle file */
#include "util/qt_dirs.hpp"
#include <vlc_intf_strings.h>
#include <vlc_modules.h>
#ifdef WIN32
  #include <vlc_charset.h> /* FromWide for Win32 */
#endif

#include <QFileDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QStackedLayout>
#include <QListView>
#include <QCompleter>
#include <QDirModel>
#include <QScrollArea>
#include <QUrl>
#include <QStringListModel>
#include <QDropEvent>

#define I_DEVICE_TOOLTIP \
    I_DIR_OR_FOLDER( N_("Select a device or a VIDEO_TS directory"), \
                     N_("Select a device or a VIDEO_TS folder") )

/* Populate a combobox with the devices matching a pattern.
   Combobox will automatically do autocompletion on the edit zone */
#define POPULATE_WITH_DEVS(ppsz_devlist, targetCombo) \
    QStringList targetCombo ## StringList = QStringList(); \
    for ( size_t i = 0; i< sizeof(ppsz_devlist) / sizeof(*ppsz_devlist); i++ ) \
        targetCombo ## StringList << QString( ppsz_devlist[ i ] ); \
    targetCombo->addItems( QDir( "/dev/" )\
        .entryList( targetCombo ## StringList, QDir::System )\
        .replaceInStrings( QRegExp("^"), "/dev/" ) \
    );

static const char psz_devModule[][8] = { "v4l2", "pvr", "dtv",
                                       "dshow", "screen", "jack" };

/**************************************************************************
 * Open Files and subtitles                                               *
 **************************************************************************/
FileOpenPanel::FileOpenPanel( QWidget *_parent, intf_thread_t *_p_intf ) :
                                OpenPanel( _parent, _p_intf ), dialogBox( NULL )
{
    /* Classic UI Setup */
    ui.setupUi( this );

    setAcceptDrops( true );

    /* Set Filters for file selection */
/*    QString fileTypes = "";
    ADD_FILTER_MEDIA( fileTypes );
    ADD_FILTER_VIDEO( fileTypes );
    ADD_FILTER_AUDIO( fileTypes );
    ADD_FILTER_PLAYLIST( fileTypes );
    ADD_FILTER_ALL( fileTypes );
    fileTypes.replace( QString(";*"), QString(" *")); */


/*    lineFileEdit = ui.fileEdit;
    //TODO later: fill the fileCompleteList with previous items played.
    QCompleter *fileCompleter = new QCompleter( fileCompleteList, this );
    fileCompleter->setModel( new QDirModel( fileCompleter ) );
    lineFileEdit->setCompleter( fileCompleter );*/
    if( var_InheritBool( p_intf, "qt-embedded-open" ) )
    {
        ui.tempWidget->hide();
        BuildOldPanel();
    }

    /* Subtitles */
    /* Deactivate the subtitles control by default. */
    ui.subFrame->setEnabled( false );

    /* Connects  */
    BUTTONACT( ui.fileBrowseButton, browseFile() );
    BUTTONACT( ui.removeFileButton, removeFile() );

    BUTTONACT( ui.subBrowseButton, browseFileSub() );
    CONNECT( ui.subCheckBox, toggled( bool ), this, toggleSubtitleFrame( bool ) );

    CONNECT( ui.fileListWidg, itemChanged( QListWidgetItem * ), this, updateMRL() );
    CONNECT( ui.subInput, textChanged( const QString& ), this, updateMRL() );
    updateButtons();
}

inline void FileOpenPanel::BuildOldPanel()
{
    /** BEGIN QFileDialog tweaking **/
    /* Use a QFileDialog and customize it because we don't want to
       rewrite it all. Be careful to your eyes cause there are a few hacks.
       Be very careful and test correctly when you modify this. */

    /* Make this QFileDialog a child of tempWidget from the ui. */
    dialogBox = new FileOpenBox( ui.tempWidget, NULL,
                                 p_intf->p_sys->filepath, "" );

    dialogBox->setFileMode( QFileDialog::ExistingFiles );
    dialogBox->setAcceptMode( QFileDialog::AcceptOpen );
    dialogBox->restoreState(
            getSettings()->value( "file-dialog-state" ).toByteArray() );

    /* We don't want to see a grip in the middle of the window, do we? */
    dialogBox->setSizeGripEnabled( false );

    /* Add a tooltip */
    dialogBox->setToolTip( qtr( "Select one or multiple files" ) );
    dialogBox->setMinimumHeight( 250 );

    // But hide the two OK/Cancel buttons. Enable them for debug.
    QDialogButtonBox *fileDialogAcceptBox =
                      dialogBox->findChildren<QDialogButtonBox*>()[0];
    fileDialogAcceptBox->hide();

    /* Ugly hacks to get the good Widget */
    //This lineEdit is the normal line in the fileDialog.
    QLineEdit *lineFileEdit = dialogBox->findChildren<QLineEdit*>()[0];
    /* Make a list of QLabel inside the QFileDialog to access the good ones */
    QList<QLabel *> listLabel = dialogBox->findChildren<QLabel*>();

    /* Hide the FileNames one. Enable it for debug */
    listLabel[1]->setText( qtr( "File names:" ) );
    /* Change the text that was uncool in the usual box */
    listLabel[2]->setText( qtr( "Filter:" ) );

    dialogBox->layout()->setMargin( 0 );
    dialogBox->layout()->setSizeConstraint( QLayout::SetNoConstraint );

    /** END of QFileDialog tweaking **/

    // Add the DialogBox to the layout
    ui.gridLayout->addWidget( dialogBox, 0, 0, 1, 3 );

    CONNECT( lineFileEdit, textChanged( const QString& ), this, updateMRL() );
    dialogBox->installEventFilter( this );
}

FileOpenPanel::~FileOpenPanel()
{
    if( dialogBox )
        getSettings()->setValue( "file-dialog-state", dialogBox->saveState() );
}

void FileOpenPanel::dragEnterEvent( QDragEnterEvent *event )
{
    event->acceptProposedAction();
}

void FileOpenPanel::dragMoveEvent( QDragMoveEvent *event )
{
    event->acceptProposedAction();
}

void FileOpenPanel::dragLeaveEvent( QDragLeaveEvent *event )
{
    event->accept();
}

void FileOpenPanel::dropEvent( QDropEvent *event )
{
    if( event->possibleActions() & Qt::CopyAction )
       event->setDropAction( Qt::CopyAction );
    else
        return;

    const QMimeData *mimeData = event->mimeData();
    foreach( const QUrl &url, mimeData->urls() )
    {
        if( url.isValid() )
        {
            QListWidgetItem *item = new QListWidgetItem(
                                         toNativeSeparators( url.toLocalFile() ),
                                         ui.fileListWidg );
            item->setFlags( Qt::ItemIsEditable | Qt::ItemIsEnabled );
            ui.fileListWidg->addItem( item );
        }
    }
    updateMRL();
    updateButtons();
    event->accept();
}

void FileOpenPanel::browseFile()
{
    QStringList files = QFileDialog::getOpenFileNames( this, qtr( "Select one or multiple files" ), p_intf->p_sys->filepath) ;
    foreach( const QString &file, files )
    {
        QListWidgetItem *item =
            new QListWidgetItem( toNativeSeparators( file ), ui.fileListWidg );
        item->setFlags( Qt::ItemIsEditable | Qt::ItemIsEnabled );
        ui.fileListWidg->addItem( item );
        savedirpathFromFile( file );
    }
    updateButtons();
    updateMRL();
}

void FileOpenPanel::removeFile()
{
    int i = ui.fileListWidg->currentRow();
    if( i != -1 )
    {
        QListWidgetItem *temp = ui.fileListWidg->takeItem( i );
        delete temp;
    }

    updateMRL();
    updateButtons();
}

/* Show a fileBrowser to select a subtitle */
void FileOpenPanel::browseFileSub()
{
    // TODO Handle selection of more than one subtitles file
    QStringList files = THEDP->showSimpleOpen( qtr("Open subtitles file"),
                           EXT_FILTER_SUBTITLE, p_intf->p_sys->filepath );

    if( files.isEmpty() ) return;
    ui.subInput->setText( toNativeSeparators( files.join(" ") ) );
    updateMRL();
}

void FileOpenPanel::toggleSubtitleFrame( bool b )
{
    ui.subFrame->setEnabled( b );

    /* Update the MRL */
    updateMRL();
}


/* Update the current MRL */
void FileOpenPanel::updateMRL()
{
    QStringList fileList;
    QString mrl;

    /* File Listing */
    if( dialogBox == NULL )
        for( int i = 0; i < ui.fileListWidg->count(); i++ )
        {
            if( !ui.fileListWidg->item( i )->text().isEmpty() )
                fileList << toURI(ui.fileListWidg->item( i )->text());
        }
    else
    {
        fileList = dialogBox->selectedFiles();
        for( int i = 0; i < fileList.count(); i++ )
            fileList[i] = toURI( fileList[i] );
    }

    /* Options */
    if( ui.subCheckBox->isChecked() &&  !ui.subInput->text().isEmpty() ) {
        mrl.append( " :sub-file=" + colon_escape( ui.subInput->text() ) );
    }

    emit methodChanged( "file-caching" );
    emit mrlUpdated( fileList, mrl );
}

/* Function called by Open Dialog when clicke on Play/Enqueue */
void FileOpenPanel::accept()
{
    if( dialogBox )
        p_intf->p_sys->filepath = dialogBox->directory().absolutePath();
    ui.fileListWidg->clear();
}

/* Function called by Open Dialog when clicked on cancel */
void FileOpenPanel::clear()
{
    ui.fileListWidg->clear();
    ui.subInput->clear();
}

/* Update buttons depending on current selection */
void FileOpenPanel::updateButtons()
{
    bool b_has_files = ( ui.fileListWidg->count() > 0 );
    ui.removeFileButton->setEnabled( b_has_files );
    ui.subCheckBox->setEnabled( b_has_files );
}

/**************************************************************************
 * Open Discs ( DVD, CD, VCD and similar devices )                        *
 **************************************************************************/
DiscOpenPanel::DiscOpenPanel( QWidget *_parent, intf_thread_t *_p_intf ) :
                                OpenPanel( _parent, _p_intf )
{
    ui.setupUi( this );

    /* Get the default configuration path for the devices */
    psz_dvddiscpath = var_InheritString( p_intf, "dvd" );
    psz_vcddiscpath = var_InheritString( p_intf, "vcd" );
    psz_cddadiscpath = var_InheritString( p_intf, "cd-audio" );

    /* State to avoid overwritting the users changes with the configuration */
    m_discType = None;

    ui.browseDiscButton->setToolTip( qtr( I_DEVICE_TOOLTIP ));
    ui.deviceCombo->setToolTip( qtr(I_DEVICE_TOOLTIP) );
    ui.deviceCombo->setInsertPolicy( QComboBox::InsertAtTop );

#ifdef WIN32 /* Disc drives probing for Windows */
    wchar_t szDrives[512];
    szDrives[0] = '\0';
    if( GetLogicalDriveStringsW( sizeof( szDrives ) - 1, szDrives ) )
    {
        wchar_t *drive = szDrives;
        UINT oldMode = SetErrorMode( SEM_FAILCRITICALERRORS );
        while( *drive )
        {
            if( GetDriveTypeW(drive) == DRIVE_CDROM )
            {
                wchar_t psz_name[512] = L"";
                GetVolumeInformationW( drive, psz_name, 511, NULL, NULL, NULL, NULL, 0 );

                QString displayName = FromWide( drive );
                if( !*psz_name ) {
                    displayName = displayName + " - "  + FromWide( psz_name );
                }

                ui.deviceCombo->addItem( displayName, FromWide( drive ) );
            }

            /* go to next drive */
            while( *(drive++) );
        }
        SetErrorMode(oldMode);
    }
#else /* Linux */
    char const * const ppsz_discdevices[] = {
        "sr*",
        "sg*",
        "scd*",
        "dvd*",
        "cd*"
    };
    QComboBox *discCombo = ui.deviceCombo; /* avoid namespacing in macro */
    POPULATE_WITH_DEVS( ppsz_discdevices, discCombo );
#endif

    /* CONNECTs */
    BUTTONACT( ui.dvdRadioButton, updateButtons() );
    BUTTONACT( ui.vcdRadioButton, updateButtons() );
    BUTTONACT( ui.audioCDRadioButton, updateButtons() );
    BUTTONACT( ui.dvdsimple, updateButtons() );
    BUTTONACT( ui.browseDiscButton, browseDevice() );
    BUTTON_SET_ACT_I( ui.ejectButton, "", toolbar/eject, qtr( "Eject the disc" ),
            eject() );

    CONNECT( ui.deviceCombo, editTextChanged( QString ), this, updateMRL());
    CONNECT( ui.deviceCombo, currentIndexChanged( QString ), this, updateMRL());
    CONNECT( ui.titleSpin, valueChanged( int ), this, updateMRL());
    CONNECT( ui.chapterSpin, valueChanged( int ), this, updateMRL());
    CONNECT( ui.audioSpin, valueChanged( int ), this, updateMRL());
    CONNECT( ui.subtitlesSpin, valueChanged( int ), this, updateMRL());

    /* Run once the updateButtons function in order to fill correctly the comboBoxes */
    updateButtons();
}

DiscOpenPanel::~DiscOpenPanel()
{
    free( psz_dvddiscpath );
    free( psz_vcddiscpath );
    free( psz_cddadiscpath );
}

void DiscOpenPanel::clear()
{
    ui.titleSpin->setValue( 0 );
    ui.chapterSpin->setValue( 0 );
    ui.subtitlesSpin->setValue( -1 );
    ui.audioSpin->setValue( -1 );
    m_discType = None;
}

#ifdef WIN32
    #define setDrive( psz_name ) {\
    int index = ui.deviceCombo->findText( qfu( psz_name ) ); \
    if( index != -1 ) ui.deviceCombo->setCurrentIndex( index );}
#else
    #define setDrive( psz_name ) {\
    ui.deviceCombo->setEditText( qfu( psz_name ) ); }
#endif

/* update the buttons according the type of device */
void DiscOpenPanel::updateButtons()
{
    if ( ui.dvdRadioButton->isChecked() )
    {
        if( m_discType != Dvd )
        {
            setDrive( psz_dvddiscpath );
            m_discType = Dvd;
        }
        ui.titleLabel->setText( qtr("Title") );
        ui.chapterLabel->show();
        ui.chapterSpin->show();
        ui.diskOptionBox_2->show();
        ui.dvdsimple->setEnabled( true );
    }
    else if ( ui.vcdRadioButton->isChecked() )
    {
        if( m_discType != Vcd )
        {
            setDrive( psz_vcddiscpath );
            m_discType = Vcd;
        }
        ui.titleLabel->setText( qtr("Entry") );
        ui.chapterLabel->hide();
        ui.chapterSpin->hide();
        ui.diskOptionBox_2->show();
        ui.dvdsimple->setEnabled( false );
    }
    else /* CDDA */
    {
        if( m_discType != Cdda )
        {
            setDrive( psz_cddadiscpath );
            m_discType = Cdda;
        }
        ui.titleLabel->setText( qtr("Track") );
        ui.chapterLabel->hide();
        ui.chapterSpin->hide();
        ui.diskOptionBox_2->hide();
        ui.dvdsimple->setEnabled( false );
    }

    updateMRL();
}

#undef setDrive

#ifndef WIN32
# define LOCALHOST ""
#else
# define LOCALHOST "/"
#endif

/* Update the current MRL */
void DiscOpenPanel::updateMRL()
{
    QString mrl;
    QString discPath;
    QStringList fileList;

    if( ui.deviceCombo->itemData( ui.deviceCombo->currentIndex() ) != QVariant::Invalid )
        discPath = ui.deviceCombo->itemData( ui.deviceCombo->currentIndex() ).toString();
    else
        discPath = ui.deviceCombo->currentText();

    /* CDDAX and VCDX not implemented. TODO ? No. */
    /* DVD */
    if( ui.dvdRadioButton->isChecked() ) {
        if( !ui.dvdsimple->isChecked() )
            mrl = "dvd://" LOCALHOST + discPath;
        else
            mrl = "dvdsimple://" LOCALHOST + discPath;

        if ( ui.titleSpin->value() > 0 ) {
            mrl += QString("@%1").arg( ui.titleSpin->value() );
            if ( ui.chapterSpin->value() > 0 ) {
                mrl+= QString(":%1").arg( ui.chapterSpin->value() );
            }
        }

    /* VCD */
    } else if ( ui.vcdRadioButton->isChecked() ) {
        mrl = "vcd://" LOCALHOST + discPath;

        if( ui.titleSpin->value() > 0 ) {
            mrl += QString("@E%1").arg( ui.titleSpin->value() );
        }

    /* CDDA */
    } else {
        mrl = "cdda://" LOCALHOST + discPath;
    }
    emit methodChanged( "disc-caching" );

    fileList << mrl; mrl = "";

    if ( ui.dvdRadioButton->isChecked() || ui.vcdRadioButton->isChecked() )
    {
        if ( ui.audioSpin->value() >= 0 ) {
            mrl += " :audio-track=" +
                QString("%1").arg( ui.audioSpin->value() );
        }
        if ( ui.subtitlesSpin->value() >= 0 ) {
            mrl += " :sub-track=" +
                QString("%1").arg( ui.subtitlesSpin->value() );
        }
    }
    else
    {
        if( ui.titleSpin->value() > 0 )
            mrl += QString(" :cdda-track=%1").arg( ui.titleSpin->value() );
    }
    emit mrlUpdated( fileList, mrl );
}

void DiscOpenPanel::browseDevice()
{
    QString dir = QFileDialog::getExistingDirectory( this,
            qtr( I_DEVICE_TOOLTIP ) );
    if( !dir.isEmpty() )
    {
        ui.deviceCombo->addItem( toNativeSepNoSlash( dir ) );
        ui.deviceCombo->setCurrentIndex( ui.deviceCombo->findText( toNativeSepNoSlash( dir ) ) );
        updateMRL();
    }

    updateMRL();
}

void DiscOpenPanel::eject()
{
    intf_Eject( p_intf, qtu( ui.deviceCombo->currentText() ) );
}

void DiscOpenPanel::accept()
{}

/**************************************************************************
 * Open Network streams and URL pages                                     *
 **************************************************************************/
NetOpenPanel::NetOpenPanel( QWidget *_parent, intf_thread_t *_p_intf ) :
                                OpenPanel( _parent, _p_intf )
{
    ui.setupUi( this );
    CONNECT( ui.urlComboBox, editTextChanged( const QString& ), this, updateMRL());

    /* */
    if( var_InheritBool( p_intf, "qt-recentplay" ) )
    {
        b_recentList = true;
        ui.urlComboBox->addItems( getSettings()->value( "Open/netMRL" ).toStringList() );
        ui.urlComboBox->setMaxCount( 10 );
    }
    else
        b_recentList = false;

    /* Use a simple validator for URLs */
    ui.urlComboBox->setValidator( new UrlValidator( this ) );
    ui.urlComboBox->setFocus();
}

NetOpenPanel::~NetOpenPanel()
{
    if( !b_recentList ) return;

    /* Create the list with the current items */
    QStringList mrlList;
    for( int i = 0; i < ui.urlComboBox->count(); i++ )
        mrlList << ui.urlComboBox->itemText( i );

    /* Clean the list... */
#if HAS_QT45
    mrlList.removeDuplicates();
#endif
    /* ...and save the 8 last entries */
    getSettings()->setValue( "Open/netMRL", mrlList );
}

void NetOpenPanel::clear()
{
    ui.urlComboBox->clear();
}

void NetOpenPanel::onAccept()
{
    if( ui.urlComboBox->findText( ui.urlComboBox->currentText() ) == -1 )
        ui.urlComboBox->insertItem( 0, ui.urlComboBox->currentText());
}

void NetOpenPanel::onFocus()
{
    ui.urlComboBox->setFocus();
    ui.urlComboBox->lineEdit()->selectAll();
}

void NetOpenPanel::updateMRL()
{
    QString url = ui.urlComboBox->lineEdit()->text();

    if( url.isEmpty() )
        return;

    emit methodChanged( qfu( "network-caching" ) );

    QStringList qsl;
    qsl << url;
    emit mrlUpdated( qsl, "" );
}

QValidator::State UrlValidator::validate( QString& str, int& ) const
{
    if( str.contains( ' ' ) )
        return QValidator::Invalid;
    if( !str.contains( "://" ) )
        return QValidator::Intermediate;
    return QValidator::Acceptable;
}

void UrlValidator::fixup( QString& str ) const
{
    str = str.trimmed();
}

/**************************************************************************
 * Open Capture device ( DVB, PVR, V4L, and similar )                     *
 **************************************************************************/
CaptureOpenPanel::CaptureOpenPanel( QWidget *_parent, intf_thread_t *_p_intf ) :
                                OpenPanel( _parent, _p_intf )
{
    isInitialized = false;
}

void CaptureOpenPanel::initialize()
{
    if( isInitialized ) return;

    msg_Dbg( p_intf, "Initialization of Capture device panel" );
    isInitialized = true;

    ui.setupUi( this );

    BUTTONACT( ui.advancedButton, advancedDialog() );

    /* Create two stacked layouts in the main comboBoxes */
    QStackedLayout *stackedDevLayout = new QStackedLayout;
    ui.cardBox->setLayout( stackedDevLayout );

    QStackedLayout *stackedPropLayout = new QStackedLayout;
    ui.optionsBox->setLayout( stackedPropLayout );

    /* Creation and connections of the WIdgets in the stacked layout */
#define addModuleAndLayouts( number, name, label, layout )            \
    QWidget * name ## DevPage = new QWidget( this );                  \
    QWidget * name ## PropPage = new QWidget( this );                 \
    stackedDevLayout->addWidget( name ## DevPage );        \
    stackedPropLayout->addWidget( name ## PropPage );      \
    layout * name ## DevLayout = new layout;                \
    layout * name ## PropLayout = new layout;               \
    name ## DevPage->setLayout( name ## DevLayout );                  \
    name ## PropPage->setLayout( name ## PropLayout );                \
    ui.deviceCombo->addItem( qtr( label ), QVariant( number ) );

#define CuMRL( widget, slot ) CONNECT( widget , slot , this, updateMRL() );

#ifdef WIN32
    /*********************
     * DirectShow Stuffs *
     *********************/
    if( module_exists( "dshow" ) ){
    addModuleAndLayouts( DSHOW_DEVICE, dshow, "DirectShow", QGridLayout );

    /* dshow Main */
    int line = 0;
    module_config_t *p_config =
        config_FindConfig( VLC_OBJECT(p_intf), "dshow-vdev" );
    vdevDshowW = new StringListConfigControl(
        VLC_OBJECT(p_intf), p_config, this, dshowDevLayout, line );
    line++;

    p_config = config_FindConfig( VLC_OBJECT(p_intf), "dshow-adev" );
    adevDshowW = new StringListConfigControl(
        VLC_OBJECT(p_intf), p_config, this, dshowDevLayout, line );
    line++;

    /* dshow Properties */
    QLabel *dshowVSizeLabel = new QLabel( qtr( "Video size" ) );
    dshowPropLayout->addWidget( dshowVSizeLabel, 0, 0 );

    dshowVSizeLine = new QLineEdit;
    dshowPropLayout->addWidget( dshowVSizeLine, 0, 1);
    dshowPropLayout->addItem( new QSpacerItem( 20, 20, QSizePolicy::Expanding ),
            1, 0, 3, 1 );

    /* dshow CONNECTs */
    CuMRL( vdevDshowW->combo, currentIndexChanged ( int ) );
    CuMRL( adevDshowW->combo, currentIndexChanged ( int ) );
    CuMRL( dshowVSizeLine, textChanged( const QString& ) );
    }
#else /* WIN32 */
    /*******
     * V4L2*
     *******/
    if( module_exists( "v4l2" ) ){
    addModuleAndLayouts( V4L2_DEVICE, v4l2, "Video for Linux 2", QGridLayout );

    char const * const ppsz_v4lvdevices[] = {
        "video*"
    };

    char const * const ppsz_v4ladevices[] = {
        "dsp*",
        "radio*"
    };

    /* V4l Main panel */
    QLabel *v4l2VideoDeviceLabel = new QLabel( qtr( "Video device name" ) );
    v4l2DevLayout->addWidget( v4l2VideoDeviceLabel, 0, 0 );

    v4l2VideoDevice = new QComboBox( this );
    v4l2VideoDevice->setEditable( true );
    POPULATE_WITH_DEVS( ppsz_v4lvdevices, v4l2VideoDevice );
    v4l2VideoDevice->clearEditText();
    v4l2DevLayout->addWidget( v4l2VideoDevice, 0, 1 );

    QLabel *v4l2AudioDeviceLabel = new QLabel( qtr( "Audio device name" ) );
    v4l2DevLayout->addWidget( v4l2AudioDeviceLabel, 1, 0 );

    v4l2AudioDevice = new QComboBox( this );
    v4l2AudioDevice->setEditable( true );
    POPULATE_WITH_DEVS( ppsz_v4ladevices, v4l2AudioDevice );
    v4l2AudioDevice->clearEditText();
    v4l2DevLayout->addWidget( v4l2AudioDevice, 1, 1 );

    /* v4l2 Props panel */
    QLabel *v4l2StdLabel = new QLabel( qtr( "Video standard" ) );
    v4l2PropLayout->addWidget( v4l2StdLabel, 0 , 0 );

    v4l2StdBox = new QComboBox;
    setfillVLCConfigCombo( "v4l2-standard", p_intf, v4l2StdBox );
    v4l2PropLayout->addWidget( v4l2StdBox, 0 , 1 );
    v4l2PropLayout->addItem( new QSpacerItem( 20, 20, QSizePolicy::Expanding ),
            1, 0, 3, 2 );

    /* v4l2 CONNECTs */
    CuMRL( v4l2VideoDevice->lineEdit(), textChanged( const QString& ) );
    CuMRL( v4l2VideoDevice,  currentIndexChanged ( int ) );
    CuMRL( v4l2AudioDevice->lineEdit(), textChanged( const QString& ) );
    CuMRL( v4l2AudioDevice,  currentIndexChanged ( int ) );
    CuMRL( v4l2StdBox,  currentIndexChanged ( int ) );
    }

    /*******
     * JACK *
     *******/
    if( module_exists( "jack" ) ){
    addModuleAndLayouts( JACK_DEVICE, jack, "JACK Audio Connection Kit",
                         QGridLayout);

    /* Jack Main panel */
    /* Channels */
    QLabel *jackChannelsLabel = new QLabel( qtr( "Channels:" ) );
    jackDevLayout->addWidget( jackChannelsLabel, 1, 0 );

    jackChannels = new QSpinBox;
    setSpinBoxFreq( jackChannels );
    jackChannels->setMaximum(255);
    jackChannels->setValue(2);
    jackChannels->setAlignment( Qt::AlignRight );
    jackDevLayout->addWidget( jackChannels, 1, 1 );

    /* Selected ports */
    QLabel *jackPortsLabel = new QLabel( qtr( "Selected ports:" ) );
    jackDevLayout->addWidget( jackPortsLabel, 0 , 0 );

    jackPortsSelected = new QLineEdit( qtr( ".*") );
    jackPortsSelected->setAlignment( Qt::AlignRight );
    jackDevLayout->addWidget( jackPortsSelected, 0, 1 );

    /* Jack Props panel */

    /* Pace */
    jackPace = new QCheckBox(qtr( "Use VLC pace" ));
    jackPropLayout->addWidget( jackPace, 1, 1 );

    /* Auto Connect */
    jackConnect = new QCheckBox( qtr( "Auto connection" ));
    jackPropLayout->addWidget( jackConnect, 1, 2 );

    /* Jack CONNECTs */
    CuMRL( jackChannels, valueChanged( int ) );
    CuMRL( jackPace, stateChanged( int ) );
    CuMRL( jackConnect, stateChanged( int ) );
    CuMRL( jackPortsSelected, textChanged( const QString& ) );
    }

    /************
     * PVR      *
     ************/
    if( module_exists( "pvr" ) ){
    addModuleAndLayouts( PVR_DEVICE, pvr, "PVR", QGridLayout );

    /* PVR Main panel */
    QLabel *pvrDeviceLabel = new QLabel( qtr( "Device name" ) );
    pvrDevLayout->addWidget( pvrDeviceLabel, 0, 0 );

    pvrDevice = new QLineEdit;
    pvrDevLayout->addWidget( pvrDevice, 0, 1 );

    QLabel *pvrRadioDeviceLabel = new QLabel( qtr( "Radio device name" ) );
    pvrDevLayout->addWidget( pvrRadioDeviceLabel, 1, 0 );

    pvrRadioDevice = new QLineEdit;
    pvrDevLayout->addWidget( pvrRadioDevice, 1, 1 );

    /* PVR props panel */
    QLabel *pvrNormLabel = new QLabel( qtr( "Norm" ) );
    pvrPropLayout->addWidget( pvrNormLabel, 0, 0 );

    pvrNormBox = new QComboBox;
    setfillVLCConfigCombo( "pvr-norm", p_intf, pvrNormBox );
    pvrPropLayout->addWidget( pvrNormBox, 0, 1 );

    QLabel *pvrFreqLabel = new QLabel( qtr( "Frequency" ) );
    pvrPropLayout->addWidget( pvrFreqLabel, 1, 0 );

    pvrFreq = new QSpinBox;
    pvrFreq->setAlignment( Qt::AlignRight );
    pvrFreq->setSuffix(" kHz");
    setSpinBoxFreq( pvrFreq );
    pvrPropLayout->addWidget( pvrFreq, 1, 1 );

    QLabel *pvrBitrLabel = new QLabel( qtr( "Bitrate" ) );
    pvrPropLayout->addWidget( pvrBitrLabel, 2, 0 );

    pvrBitr = new QSpinBox;
    pvrBitr->setAlignment( Qt::AlignRight );
    pvrBitr->setSuffix(" kHz");
    setSpinBoxFreq( pvrBitr );
    pvrPropLayout->addWidget( pvrBitr, 2, 1 );
    pvrPropLayout->addItem( new QSpacerItem( 20, 20, QSizePolicy::Expanding ),
            3, 0, 1, 1 );

    /* PVR CONNECTs */
    CuMRL( pvrDevice, textChanged( const QString& ) );
    CuMRL( pvrRadioDevice, textChanged( const QString& ) );

    CuMRL( pvrFreq, valueChanged ( int ) );
    CuMRL( pvrBitr, valueChanged ( int ) );
    CuMRL( pvrNormBox, currentIndexChanged ( int ) );
    }
#endif
    /*************
     * DVB Stuff *
     *************/
    if( module_exists( "dtv" ) ){
    addModuleAndLayouts( DTV_DEVICE, dvb, N_("TV (digital)"), QGridLayout );

    /* DVB Main */
    QLabel *dvbDeviceLabel = new QLabel( qtr( "Tuner card" ) );
    QLabel *dvbTypeLabel = new QLabel( qtr( "Delivery system" ) );

    dvbCard = new QSpinBox;
    dvbCard->setAlignment( Qt::AlignRight );
#ifdef __linux__
    dvbCard->setPrefix( "/dev/dvb/adapter" );
#endif
    dvbDevLayout->addWidget( dvbDeviceLabel, 0, 0 );
    dvbDevLayout->addWidget( dvbCard, 0, 1, 1, 2 );

    dvbc = new QRadioButton( "DVB-C" );
    dvbs = new QRadioButton( "DVB-S" );
    dvbs2 = new QRadioButton( "DVB-S2" );
    dvbt = new QRadioButton( "DVB-T" );
    dvbt2 = new QRadioButton( "DVB-T2" );
    atsc = new QRadioButton( "ATSC" );
    cqam = new QRadioButton( "Clear QAM" );
    dvbt->setChecked( true );

    dvbDevLayout->addWidget( dvbTypeLabel, 1, 0, 2, 1 );
    dvbDevLayout->addWidget( dvbc,  1, 1 );
    dvbDevLayout->addWidget( dvbs,  1, 2 );
    dvbDevLayout->addWidget( dvbs2, 2, 2 );
    dvbDevLayout->addWidget( dvbt,  1, 3 );
    dvbDevLayout->addWidget( dvbt2, 2, 3 );
    dvbDevLayout->addWidget( atsc,  1, 4 );
    dvbDevLayout->addWidget( cqam,  2, 4 );

    /* DVB Props panel */
    QLabel *dvbFreqLabel =
                    new QLabel( qtr( "Transponder/multiplex frequency" ) );
    dvbPropLayout->addWidget( dvbFreqLabel, 0, 0 );

    dvbFreq = new QSpinBox;
    dvbFreq->setAlignment( Qt::AlignRight );
    dvbFreq->setSuffix(" kHz");
    dvbFreq->setSingleStep( 1000 );
    setSpinBoxFreq( dvbFreq  );
    dvbPropLayout->addWidget( dvbFreq, 0, 1 );

    dvbSrateLabel = new QLabel( qtr( "Transponder symbol rate" ) );
    dvbPropLayout->addWidget( dvbSrateLabel, 1, 0 );

    dvbSrate = new QSpinBox;
    dvbSrate->setAlignment( Qt::AlignRight );
    dvbSrate->setSuffix(" bauds");
    setSpinBoxFreq( dvbSrate );
    dvbPropLayout->addWidget( dvbSrate, 1, 1 );

    dvbModLabel = new QLabel( qtr( "Modulation / Constellation" ) );
    dvbPropLayout->addWidget( dvbModLabel, 2, 0 );

    dvbQamBox = new QComboBox;
    dvbQamBox->addItem( qtr( "Automatic" ), qfu("QAM") );
    dvbQamBox->addItem( "256-QAM", qfu("256QAM") );
    dvbQamBox->addItem( "128-QAM", qfu("128QAM") );
    dvbQamBox->addItem( "64-QAM", qfu("64QAM") );
    dvbQamBox->addItem( "32-QAM", qfu("32QAM") );
    dvbQamBox->addItem( "16-QAM", qfu("16QAM") );
    dvbPropLayout->addWidget( dvbQamBox, 2, 1 );

    dvbPskBox = new QComboBox;
    dvbPskBox->addItem( "QPSK", qfu("QPSK") );
    dvbPskBox->addItem( "DQPSK", qfu("DQPSK") );
    dvbPskBox->addItem( "8-PSK", qfu("8PSK") );
    dvbPskBox->addItem( "16-APSK", qfu("16APSK") );
    dvbPskBox->addItem( "32-APSK", qfu("32APSK") );
    dvbPropLayout->addWidget( dvbPskBox, 2, 1 );

    dvbModLabel->hide();
    dvbQamBox->hide();
    dvbPskBox->hide();

    dvbBandLabel = new QLabel( qtr( "Bandwidth" ) );
    dvbPropLayout->addWidget( dvbBandLabel, 2, 0 );

    dvbBandBox = new QComboBox;
    setfillVLCConfigCombo( "dvb-bandwidth", p_intf, dvbBandBox );
    dvbPropLayout->addWidget( dvbBandBox, 2, 1 );

    dvbBandLabel->hide();
    dvbBandBox->hide();

    dvbPropLayout->addItem( new QSpacerItem( 20, 20, QSizePolicy::Expanding ),
            2, 0, 2, 1 );

    /* DVB CONNECTs */
    CuMRL( dvbCard, valueChanged ( int ) );
    CuMRL( dvbFreq, valueChanged ( int ) );
    CuMRL( dvbSrate, valueChanged ( int ) );
    CuMRL( dvbQamBox, currentIndexChanged ( int ) );
    CuMRL( dvbPskBox, currentIndexChanged ( int ) );
    CuMRL( dvbBandBox, currentIndexChanged ( int ) );

    BUTTONACT( dvbc, updateButtons() );
    BUTTONACT( dvbs, updateButtons() );
    BUTTONACT( dvbs2, updateButtons() );
    BUTTONACT( dvbt, updateButtons() );
    BUTTONACT( dvbt2, updateButtons() );
    BUTTONACT( atsc, updateButtons() );
    BUTTONACT( cqam, updateButtons() );
    BUTTONACT( dvbc, updateMRL() );
    BUTTONACT( dvbt, updateMRL() );
    BUTTONACT( dvbt2, updateMRL() );
    BUTTONACT( dvbs, updateMRL() );
    BUTTONACT( dvbs2, updateMRL() );
    BUTTONACT( atsc, updateMRL() );
    BUTTONACT( cqam, updateMRL() );
    }

    /**********
     * Screen *
     **********/
    addModuleAndLayouts( SCREEN_DEVICE, screen, "Desktop", QGridLayout );
    QLabel *screenLabel = new QLabel( qtr( "Your display will be "
            "opened and played in order to stream or save it." ) );
    screenLabel->setWordWrap( true );
    screenDevLayout->addWidget( screenLabel, 0, 0 );

    QLabel *screenFPSLabel = new QLabel(
            qtr( "Desired frame rate for the capture." ) );
    screenPropLayout->addWidget( screenFPSLabel, 0, 0 );

    screenFPS = new QDoubleSpinBox;
    screenFPS->setValue( 1. );
    screenFPS->setRange( .01, 100. );
    screenFPS->setAlignment( Qt::AlignRight );
    /* xgettext: frames per second */
    screenFPS->setSuffix( qtr( " f/s" ) );
    screenPropLayout->addWidget( screenFPS, 0, 1 );

    /* Screen connect */
    CuMRL( screenFPS, valueChanged( double ) );

    /* General connects */
    CONNECT( ui.deviceCombo, activated( int ) ,
             stackedDevLayout, setCurrentIndex( int ) );
    CONNECT( ui.deviceCombo, activated( int ),
             stackedPropLayout, setCurrentIndex( int ) );
    CONNECT( ui.deviceCombo, activated( int ), this, updateMRL() );
    CONNECT( ui.deviceCombo, activated( int ), this, updateButtons() );

#undef CuMRL
#undef addModuleAndLayouts
}

CaptureOpenPanel::~CaptureOpenPanel()
{
}

void CaptureOpenPanel::clear()
{
    advMRL.clear();
}

void CaptureOpenPanel::updateMRL()
{
    QString mrl = "";
    QStringList fileList;
    int i_devicetype = ui.deviceCombo->itemData(
            ui.deviceCombo->currentIndex() ).toInt();
    switch( i_devicetype )
    {
#ifdef WIN32
    case DSHOW_DEVICE:
        fileList << "dshow://";
        mrl+= " :dshow-vdev=" +
            colon_escape( QString("%1").arg( vdevDshowW->getValue() ) );
        mrl+= " :dshow-adev=" +
            colon_escape( QString("%1").arg( adevDshowW->getValue() ) )+" ";
        if( dshowVSizeLine->isModified() )
            mrl += ":dshow-size=" + dshowVSizeLine->text();
        break;
#else
    case V4L2_DEVICE:
        fileList << "v4l2://" + v4l2VideoDevice->currentText();
        mrl += ":v4l2-standard="
            + v4l2StdBox->itemData( v4l2StdBox->currentIndex() ).toString();
        mrl += " :input-slave=alsa://" + v4l2AudioDevice->currentText();
        break;
    case JACK_DEVICE:
        mrl = "jack://";
        mrl += "channels=" + QString::number( jackChannels->value() );
        mrl += ":ports=" + jackPortsSelected->text();
        fileList << mrl; mrl = "";

        if ( jackPace->isChecked() )
        {
                mrl += " :jack-input-use-vlc-pace";
        }
        if ( jackConnect->isChecked() )
        {
                mrl += " :jack-input-auto-connect";
        }
        break;
    case PVR_DEVICE:
        fileList << "pvr://";
        mrl += " :pvr-device=" + pvrDevice->text();
        mrl += " :pvr-radio-device=" + pvrRadioDevice->text();
        mrl += " :pvr-norm=" + QString::number( pvrNormBox->currentIndex() );
        if( pvrFreq->value() )
            mrl += " :pvr-frequency=" + QString::number( pvrFreq->value() );
        if( pvrBitr->value() )
            mrl += " :pvr-bitrate=" + QString::number( pvrBitr->value() );
        break;
#endif
    case DTV_DEVICE:
        if( dvbc->isChecked() ) mrl = "dvb-c://";
        else
        if( dvbs->isChecked() ) mrl = "dvb-s://";
        else
        if( dvbs2->isChecked() ) mrl = "dvb-s2://";
        else
        if( dvbt->isChecked() ) mrl = "dvb-t://";
        else
        if( dvbt2->isChecked() ) mrl = "dvb-t2://";
        else
        if( atsc->isChecked() ) mrl = "atsc://";
        else
        if( cqam->isChecked() ) mrl = "cqam://";

        mrl += "frequency=" + QString::number( dvbFreq->value() ) + "000";

        if( dvbc->isChecked() || cqam->isChecked() )
            mrl += ":modulation="
                + dvbQamBox->itemData( dvbQamBox->currentIndex() ).toString();
        if( dvbs2->isChecked() )
            mrl += ":modulation="
                + dvbPskBox->itemData( dvbPskBox->currentIndex() ).toString();
        if( dvbc->isChecked() || dvbs->isChecked() || dvbs2->isChecked() )
            mrl += ":srate=" + QString::number( dvbSrate->value() );
        if( dvbt->isChecked() || dvbt2->isChecked() )
            mrl += ":bandwidth=" +
                QString::number( dvbBandBox->itemData(
                    dvbBandBox->currentIndex() ).toInt() );

        fileList << mrl; mrl= "";
        mrl += " :dvb-adapter=" + QString::number( dvbCard->value() );
        break;
    case SCREEN_DEVICE:
        fileList << "screen://";
        mrl = " :screen-fps=" + QString::number( screenFPS->value(), 'f' );
        updateButtons();
        break;
    }
    emit methodChanged( "live-caching" );

    if( !advMRL.isEmpty() ) mrl += advMRL;

    emit mrlUpdated( fileList, mrl );
}

/**
 * Update the Buttons (show/hide) for the GUI as all device type don't
 * use the same ui. elements.
 **/
void CaptureOpenPanel::updateButtons()
{
    /*  Be sure to display the ui Elements in case they were hidden by
     *  some Device Type (like Screen://) */
    ui.optionsBox->show();
    ui.advancedButton->show();
    /* Get the current Device Number */
    int i_devicetype = ui.deviceCombo->itemData(
                                ui.deviceCombo->currentIndex() ).toInt();
    switch( i_devicetype )
    {
    case DTV_DEVICE:
        dvbSrate->hide();
        dvbSrateLabel->hide();
        dvbQamBox->hide();
        dvbPskBox->hide();
        dvbModLabel->hide();
        dvbBandBox->hide();
        dvbBandLabel->hide();

        if( dvbc->isChecked() )
        {
            dvbSrate->show();
            dvbSrateLabel->show();
            dvbQamBox->show();
            dvbModLabel->show();
        }
        else if( dvbs->isChecked() )
        {
            dvbSrate->show();
            dvbSrateLabel->show();
        }
        else if( dvbs2->isChecked() )
        {
            dvbSrate->show();
            dvbSrateLabel->show();
            dvbPskBox->show();
            dvbModLabel->show();
        }
        else if( dvbt->isChecked() || dvbt2->isChecked() )
        {
            dvbBandBox->show();
            dvbBandLabel->show();
        }
        break;
    case SCREEN_DEVICE:
        //ui.optionsBox->hide();
        ui.advancedButton->hide();
        break;
    }

    advMRL.clear();
}

void CaptureOpenPanel::advancedDialog()
{
    /* Get selected device type */
    int i_devicetype = ui.deviceCombo->itemData(
                                ui.deviceCombo->currentIndex() ).toInt();

    /* Get the corresponding module */
    module_t *p_module =
        module_find( psz_devModule[i_devicetype] );
    if( NULL == p_module ) return;

    /* Init */
    QList<ConfigControl *> controls;

    /* Get the confsize  */
    unsigned int i_confsize;
    module_config_t *p_config;
    p_config = module_config_get( p_module, &i_confsize );

    /* New Adv Prop dialog */
    adv = new QDialog( this );
    adv->setWindowTitle( qtr( "Advanced Options" ) );
    adv->setWindowRole( "vlc-advanced-options" );

    /* A main Layout with a Frame */
    QVBoxLayout *mainLayout = new QVBoxLayout( adv );
    QScrollArea *scroll = new QScrollArea;
    mainLayout->addWidget( scroll );

    QFrame *advFrame = new QFrame;
    /* GridLayout inside the Frame */
    QGridLayout *gLayout = new QGridLayout( advFrame );

    scroll->setWidgetResizable( true );
    scroll->setWidget( advFrame );

    /* Create the options inside the FrameLayout */
    for( int n = 0; n < (int)i_confsize; n++ )
    {
        module_config_t *p_item = p_config + n;
        ConfigControl *config = ConfigControl::createControl(
                        VLC_OBJECT( p_intf ), p_item, advFrame, gLayout, n );
        if ( config )
            controls.append( config );
    }

    /* Button stuffs */
    QDialogButtonBox *advButtonBox = new QDialogButtonBox( adv );
    QPushButton *closeButton = new QPushButton( qtr( "OK" ) );
    QPushButton *cancelButton = new QPushButton( qtr( "Cancel" ) );

    CONNECT( closeButton, clicked(), adv, accept() );
    CONNECT( cancelButton, clicked(), adv, reject() );

    advButtonBox->addButton( closeButton, QDialogButtonBox::AcceptRole );
    advButtonBox->addButton( cancelButton, QDialogButtonBox::RejectRole );

    mainLayout->addWidget( advButtonBox );

    /* Creation of the MRL */
    if( adv->exec() )
    {
        QString tempMRL = "";
        for( int i = 0; i < controls.count(); i++ )
        {
            ConfigControl *control = controls[i];

            tempMRL += (i ? " :" : ":");

            if( control->getType() == CONFIG_ITEM_BOOL )
                if( !(qobject_cast<VIntConfigControl *>(control)->getValue() ) )
                    tempMRL += "no-";

            tempMRL += control->getName();

            switch( control->getType() )
            {
                case CONFIG_ITEM_STRING:
                case CONFIG_ITEM_LOADFILE:
                case CONFIG_ITEM_SAVEFILE:
                case CONFIG_ITEM_DIRECTORY:
                case CONFIG_ITEM_MODULE:
                    tempMRL += colon_escape( QString("=%1").arg( qobject_cast<VStringConfigControl *>(control)->getValue() ) );
                    break;
                case CONFIG_ITEM_INTEGER:
                    tempMRL += QString("=%1").arg( qobject_cast<VIntConfigControl *>(control)->getValue() );
                    break;
                case CONFIG_ITEM_FLOAT:
                    tempMRL += QString("=%1").arg( qobject_cast<VFloatConfigControl *>(control)->getValue() );
                    break;
            }
        }
        advMRL = tempMRL;
        updateMRL();
        msg_Dbg( p_intf, "%s", qtu( advMRL ) );
    }
    qDeleteAll( controls );
    delete adv;
    module_config_free( p_config );
}

