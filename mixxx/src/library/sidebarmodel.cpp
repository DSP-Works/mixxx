#include <QtDebug>
#include <QUrl>
#include <QApplication>

#include "library/libraryfeature.h"
#include "library/sidebarmodel.h"
#include "library/treeitem.h"

SidebarModel::SidebarModel(QObject* parent)
        : QAbstractItemModel(parent),
          m_iDefaultSelectedIndex(0) {
}

SidebarModel::~SidebarModel() {

}

void SidebarModel::addLibraryFeature(LibraryFeature* feature) {
    m_sFeatures.push_back(feature);
    connect(feature, SIGNAL(featureUpdated()), this, SLOT(refreshData()));
    QAbstractItemModel* model = feature->getChildModel();

    connect(model, SIGNAL(modelReset()),
            this, SLOT(slotModelReset()));
    connect(model, SIGNAL(dataChanged(const QModelIndex&,const QModelIndex&)),
            this, SLOT(slotDataChanged(const QModelIndex&,const QModelIndex&)));
    
    connect(model, SIGNAL(rowsAboutToBeInserted(const QModelIndex&, int, int)),
            this, SLOT(slotRowsAboutToBeInserted(const QModelIndex&, int, int)));
    connect(model, SIGNAL(rowsAboutToBeRemoved(const QModelIndex&, int, int)),
            this, SLOT(slotRowsAboutToBeRemoved(const QModelIndex&, int, int)));
    connect(model, SIGNAL(rowsInserted(const QModelIndex&, int, int)),
            this, SLOT(slotRowsInserted(const QModelIndex&, int, int)));
    connect(model, SIGNAL(rowsRemoved(const QModelIndex&, int, int)),
            this, SLOT(slotRowsRemoved(const QModelIndex&, int, int)));
    
}

QModelIndex SidebarModel::getDefaultSelection() {
    if (m_sFeatures.size() == 0)
        return QModelIndex();
    return createIndex(m_iDefaultSelectedIndex, 0, (void*)this);
}

void SidebarModel::setDefaultSelection(unsigned int index)
{
    m_iDefaultSelectedIndex = index;
}

void SidebarModel::activateDefaultSelection() {
    if (m_sFeatures.size() > 0) {
        m_sFeatures[m_iDefaultSelectedIndex]->activate();
    }
}

void SidebarModel::refreshData()
{
    //Reset all the model indices and refresh all the data.
    //TODO: Could do something nicer when a feature's children change,
    //      but the features know nothing about their model indices,
    //      so they can't do stuff like beginInsertRow() to help the
    //      model manage the indices.
    //reset();
}

QModelIndex SidebarModel::index(int row, int column,
                                const QModelIndex& parent) const {
    // qDebug() << "SidebarModel::index row=" << row
      //       << "column=" << column << "parent=" << parent.data();
    if (parent.isValid()) {
        /* If we have selected the root of a library feature at position 'row'
         * its internal pointer is the current sidebar object model
         * we return its associated childmodel
         */
        if (parent.internalPointer() == this) {
            const QAbstractItemModel* childModel = m_sFeatures[parent.row()]->getChildModel();
            QModelIndex childIndex = childModel->index(row, column);
            TreeItem* tree_item = (TreeItem*)childIndex.internalPointer();
            if (childIndex.isValid()) 
            {
     
                return createIndex(childIndex.row(), childIndex.column(), (void*)tree_item);
            } 
            else 
            {
               
                return QModelIndex();
            }
        } 
        else 
        {
            // We have selected an item within the childmodel
            // This item has always an internal pointer of (sub)type TreeItem
            TreeItem* tree_item = (TreeItem*)parent.internalPointer();
            return createIndex(row, column, (void*) tree_item->child(row));
        }
    }
    return createIndex(row, column, (void*)this);
}

QModelIndex SidebarModel::parent(const QModelIndex& index) const {
    //qDebug() << "SidebarModel::parent index=" << index.data();
    if (index.isValid()) {
        /* If we have selected the root of a library feature
         * its internal pointer is the current sidebar object model
         * A root library feature has no parent and thus we return 
         * an invalid QModelIndex
         */
        if (index.internalPointer() == this) {
            
            return QModelIndex();
        } 
        else 
        {
            TreeItem* tree_item = (TreeItem*)index.internalPointer();
            //if we have selected an item at the first level of a childnode
            if(tree_item->parent()->data() == "$root"){
                //qDebug() << "Parent== root";
                LibraryFeature* feature = tree_item->getFeature();
                for (int i = 0; i < m_sFeatures.size(); ++i) 
                {
                    if (feature == m_sFeatures[i]) {
                         //create a ModelIndex for parent 'this' having a library feature at position 'i' 
                        return createIndex(i, 0, (void*)this);
                    }
                }
            }
            //if we have selected an item at some deeper level of a childnode
            return createIndex(tree_item->parent()->row(), 0 , tree_item->parent());
        }
    }
    return QModelIndex();
}

int SidebarModel::rowCount(const QModelIndex& parent) const {
    //qDebug() << "SidebarModel::rowCount parent=" << parent.data();
    if (parent.isValid()) {
        if (parent.internalPointer() == this) {
     
            return m_sFeatures[parent.row()]->getChildModel()->rowCount();
        } 
        else {
            //We support tree models deeper than 1 level
            TreeItem* tree_item = (TreeItem*)parent.internalPointer();
            
            return tree_item->childCount();
        }
    }
    return m_sFeatures.size();
}

int SidebarModel::columnCount(const QModelIndex& parent) const {
    //qDebug() << "SidebarModel::columnCount parent=" << parent;
    // TODO(rryan) will we ever have columns? I don't think so.
    return 1;
}

QVariant SidebarModel::data(const QModelIndex& index, int role) const {
    // qDebug("SidebarModel::data row=%d column=%d pointer=%8x, role=%d",
    //        index.row(), index.column(), index.internalPointer(), role);
    if (index.isValid()) {
        if (index.internalPointer() == this) 
        {
            if (role == Qt::DisplayRole) {
                return m_sFeatures[index.row()]->title();
            } 
            else if (role == Qt::DecorationRole) {
                return m_sFeatures[index.row()]->getIcon();
            }
        } 
        else {
            TreeItem* tree_item = (TreeItem*)index.internalPointer();
            
            if (role == Qt::DisplayRole) {
                return tree_item->data();
            }
            else if (role == Qt::DecorationRole) {
                return tree_item->getIcon();
            }
            
        }
    }
    return QVariant();
}

void SidebarModel::clicked(const QModelIndex& index) {
    //qDebug() << "SidebarModel::clicked() index=" << index;


    //We use clicked() for keyboard and mouse control, and the
    //following code breaks that for us:
    /*if (QApplication::mouseButtons() != Qt::LeftButton) {
        return;
    }*/

    if (index.isValid()) {
        if (index.internalPointer() == this) {
            m_sFeatures[index.row()]->activate();
        } 
        else 
        {
            TreeItem* tree_item = (TreeItem*)index.internalPointer();
            LibraryFeature* feature = tree_item->getFeature();
            feature->activateChild(index);
        }
    }
}

void SidebarModel::rightClicked(const QPoint& globalPos, const QModelIndex& index) {
    //qDebug() << "SidebarModel::rightClicked() index=" << index;
    if (index.isValid()) 
    {
        if (index.internalPointer() == this) 
        {
            m_sFeatures[index.row()]->activate();
            m_sFeatures[index.row()]->onRightClick(globalPos);
        }
        else 
        {
            TreeItem* tree_item = (TreeItem*)index.internalPointer();
            LibraryFeature* feature = tree_item->getFeature();
            feature->activateChild(index);
            feature->onRightClickChild(globalPos, index);
           
        }
    }
}

bool SidebarModel::dropAccept(const QModelIndex& index, QUrl url)
{
    //qDebug() << "SidebarModel::dropAccept() index=" << index << url;
    if (index.isValid()) {
        if (index.internalPointer() == this) 
        {
            return m_sFeatures[index.row()]->dropAccept(url);
        } 
        else 
        {
            TreeItem* tree_item = (TreeItem*)index.internalPointer();
            LibraryFeature* feature = tree_item->getFeature();
            return feature->dropAcceptChild(index, url);
            
        }
    }

    return false;
}

bool SidebarModel::dragMoveAccept(const QModelIndex& index, QUrl url)
{
    //qDebug() << "SidebarModel::dragMoveAccept() index=" << index << url;
    if (index.isValid()) {
        if (index.internalPointer() == this) 
        {
            return m_sFeatures[index.row()]->dragMoveAccept(url);
        } 
        else 
        {
            
            TreeItem* tree_item = (TreeItem*)index.internalPointer();
            LibraryFeature* feature = tree_item->getFeature();
            return feature->dragMoveAcceptChild(index, url);
        }
    }
    return false;
}

QModelIndex SidebarModel::translateSourceIndex(const QModelIndex& index) {
    QModelIndex translatedIndex;
    const QAbstractItemModel* model = (QAbstractItemModel*)sender();
    Q_ASSERT(model);
    if (index.isValid()) {
        translatedIndex = createIndex(index.row(), index.column(),
                                      (void*)model);
    } else {
        for (int i = 0; i < m_sFeatures.size(); ++i) {
            if (m_sFeatures[i]->getChildModel() == model) {
                translatedIndex = createIndex(i, 0, (void*)this);
            }
        }
    }
    return translatedIndex;
}

void SidebarModel::slotDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight) {
    //qDebug() << "slotDataChanged topLeft:" << topLeft << "bottomRight:" << bottomRight;
}

void SidebarModel::slotRowsAboutToBeInserted(const QModelIndex& parent, int start, int end) {
    //qDebug() << "slotRowsABoutToBeInserted" << parent << start << end;
    
    QModelIndex newParent = translateSourceIndex(parent);        
    beginInsertRows(newParent, start, end);
}

void SidebarModel::slotRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) {
    //qDebug() << "slotRowsABoutToBeRemoved" << parent << start << end;
      
    QModelIndex newParent = translateSourceIndex(parent);
    beginRemoveRows(newParent, start, end);
}

void SidebarModel::slotRowsInserted(const QModelIndex& parent, int start, int end) {
   // qDebug() << "slotRowsInserted" << parent << start << end;
    //QModelIndex newParent = translateSourceIndex(parent);
    endInsertRows();
}

void SidebarModel::slotRowsRemoved(const QModelIndex& parent, int start, int end) {
    //qDebug() << "slotRowsRemoved" << parent << start << end;
    //QModelIndex newParent = translateSourceIndex(parent);
    endRemoveRows();
}

void SidebarModel::slotModelReset() {
    // If a child model is reset, we can't really do anything but reset(). This
    // will close any open items.
    reset();
}
