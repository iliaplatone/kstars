/*
    SPDX-FileCopyrightText: 2017 Jasem Mutlaq <mutlaqja@ikarustech.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "ui_filtersettings.h"
#include "ekos/ekos.h"

#include <indi/indifilterwheel.h>
#include <indi/indifocuser.h>
#include <oal/filter.h>

#include <QDialog>
#include <QSqlDatabase>
#include <QQueue>
#include <QPointer>
#include <QStandardItemModel>
#include <QProgressBar>
#include <QStatusBar>

class QSqlTableModel;
class ComboDelegate;
class NotEditableDelegate;
class NotEditableDelegate2dp;
class DoubleDelegate;
class IntegerDelegate;
class ToggleDelegate;

namespace Ekos
{

class FilterManager : public QDialog, public Ui::FilterSettings
{
        Q_OBJECT
    public:

        typedef enum
        {
            CHANGE_POLICY    = 1 << 0,
            OFFSET_POLICY    = 1 << 1,
            AUTOFOCUS_POLICY = 1 << 2,
            ALL_POLICIES     = CHANGE_POLICY | OFFSET_POLICY | AUTOFOCUS_POLICY,
            NO_AUTOFOCUS_POLICY = CHANGE_POLICY | OFFSET_POLICY
        } FilterPolicy;

        enum
        {
            FM_LABEL = 4,
            FM_EXPOSURE,
            FM_OFFSET,
            FM_AUTO_FOCUS,
            FM_LOCK_FILTER,
            FM_LAST_AF_SOLUTION,
            FM_LAST_AF_TEMP,
            FM_LAST_AF_ALT,
            FM_TICKS_PER_TEMP,
            FM_TICKS_PER_ALT,
            FM_WAVELENGTH
        };

        enum
        {
            BFO_FILTER = 0,
            BFO_OFFSET,
            BFO_LOCK,
            BFO_NUM_FOCUS_RUNS,
            BFO_AF_RUN_1
        };

        typedef enum
        {
            BFO_INIT,
            BFO_RUN,
            BFO_SAVE,
            BFO_STOP
        } BFOButtonState;

        FilterManager(QWidget *parent = nullptr);

        QJsonObject toJSON();
        void setFilterData(const QJsonObject &settings);

        void refreshFilterModel();

        QStringList getFilterLabels(bool forceRefresh = false);

        int getFilterPosition(bool forceRefresh = false);

        // The target position and offset
        int getTargetFilterPosition()
        {
            return targetFilterPosition;
        }
        int getTargetFilterOffset()
        {
            return targetFilterOffset;
        }

        /**
         * @brief setFilterAbsoluteFocusDetails set params from successful autofocus run
         * @param index filter index
         * @param focusPos the position of the focus solution
         * @param focusTemp the temperature at the time of the focus run
         * @param focusAlt the altitude at the time of the focus run
         * @return whether function worked or not
         */
        bool setFilterAbsoluteFocusDetails(int index, int focusPos, double focusTemp, double focusAlt);

        /**
         * @brief getFilterAbsoluteFocusDetails get params from the last successful autofocus run
         * @param name filter name
         * @param focusPos the position of the focus solution
         * @param focusTemp the temperature at the time of the focus run
         * @param focusAlt the altitude at the time of the focus run
         * @return whether function worked or not
         */
        bool getFilterAbsoluteFocusDetails(const QString &name, int &focusPos, double &focusTemp, double &focusAlt) const;

        // Set absolute focus position, if supported, to the current filter absolute focus position.
        bool syncAbsoluteFocusPosition(int index);

        /**
         * @brief getFilterExposure Get optimal exposure time for the specified filter
         * @param name filter to obtain exposure time for
         * @return exposure time in seconds.
         */
        double getFilterExposure(const QString &name = QString()) const;
        bool setFilterExposure(int index, double exposure);

        /**
         * @brief setFilterOffset set the offset for the specified filter
         * @param color of the filter
         * @param the new filter offset
         * @return whether or not the operation was successful
         */
        bool setFilterOffset(QString color, int offset);

        /**
         * @brief getFilterWavelength Get mid-point wavelength for the specified filter
         * @param name filter to obtain exposure time for
         * @return wavelength in nm.
         */
        int getFilterWavelength(const QString &name = QString()) const;

        /**
         * @brief getFilterTicksPerTemp gets the ticks per degree C
         * @param name filter to obtain exposure time for
         * @return ticks / degree C
         */
        double getFilterTicksPerTemp(const QString &name = QString()) const;

        /**
         * @brief getFilterTicksPerAlt gets the ticks per degree of altitude
         * @param name filter to obtain exposure time for
         * @return ticks / degree Alt
         */
        double getFilterTicksPerAlt(const QString &name = QString()) const;

        /**
         * @brief getFilterLock Return filter that should be used when running autofocus for the supplied filter
         * For example, "Red" filter can be locked to use "Lum" when doing autofocus. "Green" filter can be locked to "--"
         * which means that no filter change is necessary.
         * @param name filter which we need to query its locked filter.
         * @return locked filter. "--" indicates no locked filter and whatever current filter should be used.
         *
         */
        QString getFilterLock(const QString &name) const;
        bool setFilterLock(int index, QString name);

        /**
         * @brief setCurrentFilterWheel Set the FilterManager active filter wheel.
         * @param filter pointer to filter wheel device
         */
        void setFilterWheel(ISD::FilterWheel *filter);
        ISD::FilterWheel *filterWheel() const
        {
            return m_FilterWheel;
        }

        /**
         * @brief setFocusReady Set whether a focuser device is active and in use.
         * @param enabled true if focus is ready, false otherwise.
         */
        void setFocusReady(bool enabled)
        {
            m_FocusReady = enabled;
        }


        /**
         * @brief applyFilterFocusPolicies Check if we need to apply any filter policies for focus operations.
         */
        void applyFilterFocusPolicies();

        /**
         * @brief autoFocusComplete Used by build filter offsets to process the completion of an AF run.
         * @param completionState was the AF run successful
         * @param currentPosition If the AF run was successful this is the focus point
         */
        void autoFocusComplete(FocusState completionState, int currentPosition);

    public slots:
        // Position. if applyPolicy is true then all filter offsets and autofocus & lock policies are applied.
        bool setFilterPosition(uint8_t position, Ekos::FilterManager::FilterPolicy policy = ALL_POLICIES);
        // Change filter names
        bool setFilterNames(const QStringList &newLabels);
        // Offset Request completed
        void setFocusOffsetComplete();
        // Remove Device
        void removeDevice(const QSharedPointer<ISD::GenericDevice> &device);
        // Refresh Filters after model update
        void reloadFilters();
        // Resize the dialog to the contents
        void resizeDialog();
        // Focus Status
        void setFocusStatus(Ekos::FocusState focusState);
        // Set absolute focus position
        void setFocusAbsolutePosition(int value)
        {
            m_FocusAbsPosition = value;
        }
        // Inti filter property after connection
        void refreshFilterProperties();

    signals:
        // Emitted only when there is a change in the filter slot number
        void positionChanged(int);
        // Emitted when filter change operation completed successfully including any focus offsets or auto-focus operation
        void labelsChanged(QStringList);
        // Emitted when filter exposure duration changes
        void exposureChanged(double);
        // Emitted when filter change completed including all required actions
        void ready();
        // Emitted when operation fails
        void failed();
        // Status signal
        void newStatus(Ekos::FilterState state);
        // Check Focus
        void checkFocus(double);
        // Run AutoFocus
        void runAutoFocus(bool buildOffsets);
        // Abort AutoFocus
        void abortAutoFocus();
        // New Focus offset requested
        void newFocusOffset(int value, bool useAbsoluteOffset);
        // database was updated
        void updated();
        // Filter ticks per degree of temperature changed
        void ticksPerTempChanged();
        // Filter ticks per degree of altitude changed
        void ticksPerAltChanged();
        // Filter wavelength changed
        void wavelengthChanged();

    private slots:
        void updateProperty(INDI::Property prop);
        void processDisconnect();
        void itemChanged(QStandardItem *item);

    private:

        // Filter Wheel Devices
        ISD::FilterWheel *m_FilterWheel = { nullptr };

        // Position and Labels
        QStringList m_currentFilterLabels;
        int m_currentFilterPosition = { -1 };
        double m_currentFilterExposure = { -1 };

        // Filter Structure
        QList<OAL::Filter *> m_ActiveFilters;
        OAL::Filter *targetFilter = { nullptr };
        OAL::Filter *currentFilter = { nullptr };
        bool m_useTargetFilter = { false };

        // Autofocus retries
        uint8_t retries = { 0 };

        int16_t lastFilterOffset { 0 };

        // Table model
        QSqlTableModel *m_FilterModel = { nullptr };

        // INDI Properties of current active filter
        ITextVectorProperty *m_FilterNameProperty { nullptr };
        INumberVectorProperty *m_FilterPositionProperty { nullptr };
        ISwitchVectorProperty *m_FilterConfirmSet { nullptr };

        // Accessor function to return filter pointer for the passed in name.
        // nullptr is returned if there isn't a match
        OAL::Filter * getFilterByName(const QString &name) const;

        // Operation stack
        void buildOperationQueue(FilterState operation);
        bool executeOperationQueue();
        bool executeOneOperation(FilterState operation);

        // Update model
        void syncDBToINDI();

        // Get the list of possible lock filters to set in the combo box.
        // The list excludes filters already setup with a lock to prevent nested dependencies
        QStringList getLockDelegates();

        // Operation Queue
        QQueue<FilterState> operationQueue;

        FilterState state = { FILTER_IDLE };

        int targetFilterPosition { -1 };
        int targetFilterOffset { - 1 };


        bool m_FocusReady { false };
        bool m_FocusAbsPositionPending { false};
        int m_FocusAbsPosition { -1 };

        // Delegates
        QPointer<ComboDelegate> lockDelegate;
        QPointer<NotEditableDelegate> noEditDelegate;
        QPointer<NotEditableDelegate2dp> noEditDelegate2dp;
        QPointer<DoubleDelegate> exposureDelegate;
        QPointer<IntegerDelegate> offsetDelegate;
        QPointer<ToggleDelegate> useAutoFocusDelegate;
        QPointer<DoubleDelegate> ticksPerTempDelegate;
        QPointer<DoubleDelegate> ticksPerAltDelegate;
        QPointer<IntegerDelegate> wavelengthDelegate;

        // Policies
        FilterPolicy m_Policy = { ALL_POLICIES };

        bool m_ConfirmationPending { false };

        // The following functions and members relate to the Build Filter Offsets dialog

        typedef struct
        {
            QString color;
            bool changeFilter;
            int numAFRun;
        } buildOffsetsQItem;

        // Function to initialise resources for the build filter offsers dialog
        void initBuildFilterOffsets();
        // Setup the table widget
        void setupBuildFilterOffsetsTable();
        // Set the buttons state
        void setBuildFilterOffsetsButtons(BFOButtonState state);
        // Function to manage Build Offsets Dialog
        void buildFilterOffsets();
        // Function to setup the work required to build the offsets
        void buildTheOffsets();
        // Function to stop in-flight processing, e.g AF runs
        void stopProcessing();
        // Function to persist the calculated filter offsets
        void saveTheOffsets();
        // Function to confirm user wants to close the dialog without saving results
        void close();
        // When all automated processing is  complete allow some cells to be editable
        void setCellsEditable();
        // Function to call Autofocus to build the filter offsets
        void runBuildOffsets();
        // Function to process a filter change event
        void buildTheOffsetsTaskComplete();
        // Resize the dialog
        void buildOffsetsDialogResize();
        // Calculate the average of the AF runs
        void calculateAFAverage(int row, int col);
        // Calculate the new offset for the filter
        void calculateOffset(int row);
        // Process the passed in Q item
        void processQItem(buildOffsetsQItem qitem);

        QStandardItemModel *m_BFOModel;
        QTableView *m_BFOView;

        QVector <QString> m_lockFilters, m_dependantFilters;
        QVector <int> m_dependantOffset;

        //buildOffsetsQItem m_qItem;

        QQueue<buildOffsetsQItem> m_buildOffsetsQ;
        buildOffsetsQItem m_qItemInProgress;
        QDialog *m_buildOffsetsDialog;
        QDialogButtonBox *m_buildOffsetButtonBox;
        QProgressBar *m_buildOffsetsProgressBar;
        QStatusBar *m_StatusBar;

        bool m_inBuildOffsets { false };
        int m_rowIdx { 0 };
        int m_colIdx { 0 };
        QPushButton *m_runButton;
        QPushButton *m_stopButton;
        bool m_problemFlag { false };
        bool m_stopFlag { false };
        bool m_abortAFPending { false };
        bool m_tableInEditMode {false};
};

}
