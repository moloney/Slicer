/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Qt includes
#include <QDebug>
#include <QInputDialog>
#include <QModelIndex>

// qMRMLWidgets
#include "qMRMLSceneModelHierarchyModel.h"

// SlicerQt includes
#include "qSlicerModelsModuleWidget.h"
#include "ui_qSlicerModelsModule.h"

// MRML includes
#include "vtkMRMLModelHierarchyNode.h"
#include "vtkMRMLModelDisplayNode.h"
#include "vtkMRMLScene.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Models
class qSlicerModelsModuleWidgetPrivate: public Ui_qSlicerModelsModule
{
public:
  qSlicerModelsModuleWidgetPrivate();
  QAction *InsertHierarchyAction;
  QAction *DeleteMultipleNodesAction;
  QAction *RenameMultipleNodesAction;
};

//-----------------------------------------------------------------------------
// qSlicerModelsModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerModelsModuleWidgetPrivate::qSlicerModelsModuleWidgetPrivate()
{
  this->InsertHierarchyAction = 0;
  this->DeleteMultipleNodesAction = 0;
  this->RenameMultipleNodesAction = 0;
}

//-----------------------------------------------------------------------------
// qSlicerModelsModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerModelsModuleWidget::qSlicerModelsModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerModelsModuleWidgetPrivate )
{
}

//-----------------------------------------------------------------------------
qSlicerModelsModuleWidget::~qSlicerModelsModuleWidget()
{
  // set the mrml scene to null so that stop observing it for events
  this->setMRMLScene(0);
}

//-----------------------------------------------------------------------------
void qSlicerModelsModuleWidget::setup()
{
  Q_D(qSlicerModelsModuleWidget);
  
  d->setupUi(this);
  d->ClipModelsNodeComboBox->setVisible(false);

  this->updateTreeViewModel();

  // turn of setting of size to visible indexes to allow drag scrolling
  d->ModelHierarchyTreeView->setFitSizeToVisibleIndexes(false);
  
  // add an add hierarchy right click action on the scene and hierarchy nodes
  connect(d->ModelHierarchyTreeView,  SIGNAL(currentNodeChanged(vtkMRMLNode*)),
          this, SLOT(onCurrentNodeChanged(vtkMRMLNode*)) );
  bool expRet = connect(d->ModelHierarchyTreeView, SIGNAL(expanded(QModelIndex)),
          this, SLOT(onExpanded(QModelIndex)));
  bool colRet = connect(d->ModelHierarchyTreeView, SIGNAL(collapsed(QModelIndex)),
          this, SLOT(onCollapsed(QModelIndex)));
  if (!expRet)
    {
    qDebug() << "Connecting expanded failed";
    }
  if (!colRet)
    {
    qDebug() << "Connecting collapsed failed";
    }
  
  d->InsertHierarchyAction = new QAction(tr("Insert hierarchy"), this);
  d->ModelHierarchyTreeView->prependSceneMenuAction(d->InsertHierarchyAction);
  connect(d->InsertHierarchyAction, SIGNAL(triggered()),
          this, SLOT(insertHierarchyNode()));

  // customize the right click menu to offer a delete multiple nodes option,
  // and deal with the hierarchies associated with model nodes
  d->DeleteMultipleNodesAction = new QAction(tr("Delete Model(s)"), this);
  d->DeleteMultipleNodesAction->setToolTip(tr("Delete one or more models, along with the 1:1 hierarchy nodes that may be associated with them"));
  d->ModelHierarchyTreeView->appendNodeMenuAction(d->DeleteMultipleNodesAction);
  connect(d->DeleteMultipleNodesAction, SIGNAL(triggered()),
          this, SLOT(deleteMultipleModels()));

  // customise the right click menu to offer a rename multiple nodes option
  d->RenameMultipleNodesAction = new QAction(tr("Rename Multiple Models"), this);
  d->ModelHierarchyTreeView->appendNodeMenuAction(d->RenameMultipleNodesAction);
  connect(d->RenameMultipleNodesAction, SIGNAL(triggered()),
          this, SLOT(renameMultipleModels()));
  
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
void qSlicerModelsModuleWidget::updateTreeViewModel()
{
  Q_D(qSlicerModelsModuleWidget);

  d->ModelHierarchyTreeView->setSceneModelType(QString("ModelHierarchy"));
  
  qobject_cast<qMRMLSceneModelHierarchyModel*>(
    d->ModelHierarchyTreeView->sceneModel())->setIDColumn(-1);
  qobject_cast<qMRMLSceneModelHierarchyModel*>(
    d->ModelHierarchyTreeView->sceneModel())->setColorColumn(1);
  qobject_cast<qMRMLSceneModelHierarchyModel*>(
    d->ModelHierarchyTreeView->sceneModel())->setOpacityColumn(2);

  qobject_cast<qMRMLSceneModelHierarchyModel*>(
    d->ModelHierarchyTreeView->sceneModel())->setColumnCount(3);
  d->ModelHierarchyTreeView->header()->setStretchLastSection(false);
  d->ModelHierarchyTreeView->header()->setResizeMode(0, QHeaderView::Stretch);
  d->ModelHierarchyTreeView->header()->setResizeMode(1, QHeaderView::ResizeToContents);
  d->ModelHierarchyTreeView->header()->setResizeMode(2, QHeaderView::ResizeToContents);

  d->ModelHierarchyTreeView->sortFilterProxyModel()->setHideChildNodeTypes(
    QStringList() << "vtkMRMLFiberBundleNode" << "vtkMRMLAnnotationNode");
  d->ModelHierarchyTreeView->sortFilterProxyModel()->setShowHiddenForTypes(
    QStringList() << "vtkMRMLModelHierarchyNode");

//  qDebug() << "qSlicerModelsModuleWidget::updateTreeViewModel done";
}

//-----------------------------------------------------------------------------
void qSlicerModelsModuleWidget::insertHierarchyNode()
{
  Q_D(qSlicerModelsModuleWidget);
  
  vtkMRMLModelHierarchyNode *modelHierarchyNode = vtkMRMLModelHierarchyNode::New();

  vtkMRMLNode* parent = vtkMRMLNode::SafeDownCast(d->ModelHierarchyTreeView->currentNode());
  if (parent)
    {
    modelHierarchyNode->SetParentNodeID(parent->GetID());
    }
  this->mrmlScene()->AddNode(modelHierarchyNode);
  modelHierarchyNode->SetName(this->mrmlScene()->GetUniqueNameByString("Model Hierarchy"));

  // also add a display node to the hierarchy node for use when the hierarchy is collapsed
  vtkMRMLModelDisplayNode *modelDisplayNode = vtkMRMLModelDisplayNode::New();
  if (modelDisplayNode)
    {
    this->mrmlScene()->AddNode(modelDisplayNode);
    modelHierarchyNode->SetAndObserveDisplayNodeID(modelDisplayNode->GetID());

    modelDisplayNode->Delete();
    }
  modelHierarchyNode->Delete();
}

//-----------------------------------------------------------------------------
void qSlicerModelsModuleWidget::deleteMultipleModels()
{
  Q_D(qSlicerModelsModuleWidget);

  if (!this->mrmlScene())
    {
    qWarning("No mrml scene set, cannot delete models");
    return;
    }
  
  // get all rows where column 2, the model name, is selected
  int nameColumn = d->ModelHierarchyTreeView->sceneModel()->nameColumn();
  QModelIndexList indexList = d->ModelHierarchyTreeView->selectionModel()->selectedRows(nameColumn);
  // have to build up a list of nodes to delete, as the node from index call will
  // fail as the tree is updated while the nodes are being deleted.
  QStringList modelIDsToDelete;
  for (int i = 0; i < indexList.size(); ++i)
    {
    QModelIndex index = indexList.at(i);
    vtkMRMLNode *nodeToDelete =  d->ModelHierarchyTreeView->sortFilterProxyModel()->mrmlNodeFromIndex(index);
    if (nodeToDelete && nodeToDelete->GetID())
      {
      modelIDsToDelete << nodeToDelete->GetID();
      }
    else
      {
      qWarning() << "selection index " << i << " has no model";
      }
    }
  for (int i = 0; i < modelIDsToDelete.size(); i++)
    {
    QString modelID = modelIDsToDelete.at(i);
    vtkMRMLNode *mrmlNode = this->mrmlScene()->GetNodeByID(modelID.toLatin1());
    if (mrmlNode && mrmlNode->IsA("vtkMRMLModelNode"))
      {
      // get the model hierarchy node and delete it
      vtkMRMLHierarchyNode *hnode = vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(mrmlNode->GetScene(), mrmlNode->GetID());
      if (hnode)
        {
        //qDebug() << i << ": removing hierarchy " << (hnode ? hnode->GetID() : "null");
        this->mrmlScene()->RemoveNode(hnode);
        }
      // remove the model node
      //qDebug() << i << ": removing model node " << mrmlNode->GetName() << ", id = " << mrmlNode->GetID();
      this->mrmlScene()->RemoveNode(mrmlNode);
      }
    else
      {
      qWarning() << "Unable to delete model using node id" << qPrintable(modelID);
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerModelsModuleWidget::renameMultipleModels()
{
  Q_D(qSlicerModelsModuleWidget);

  if (!this->mrmlScene())
    {
    qWarning("No mrml scene set, cannot rename models");
    return;
    }
  
  // get all rows where column 2, the model name, is selected
  int nameColumn = d->ModelHierarchyTreeView->sceneModel()->nameColumn();
  QModelIndexList indexList = d->ModelHierarchyTreeView->selectionModel()->selectedRows(nameColumn);
  for (int i = 0; i < indexList.size(); ++i)
    {
    QModelIndex index = indexList.at(i);
    vtkMRMLNode *mrmlNode =  d->ModelHierarchyTreeView->sortFilterProxyModel()->mrmlNodeFromIndex(index);
    if (mrmlNode && mrmlNode->IsA("vtkMRMLModelNode"))
      {
      // pop up an entry box for the new name, with the old name as default
      QString oldName = mrmlNode->GetName();
      bool ok = false;
      QString newName = QInputDialog::getText(
                        this, "Rename " + oldName, "Old name: " + oldName + "\nNew name:",
                        QLineEdit::Normal, oldName, &ok);
      if (ok)
        {
        mrmlNode->SetName(newName.toLatin1());
        }
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerModelsModuleWidget::onCurrentNodeChanged(vtkMRMLNode* newCurrentNode)
{
  Q_D(qSlicerModelsModuleWidget);

  // only allow adding hierarchies when right click on hierarchies
  vtkMRMLModelHierarchyNode* hierarchyNode =
    vtkMRMLModelHierarchyNode::SafeDownCast(newCurrentNode);
  if (hierarchyNode)
    {
    d->ModelHierarchyTreeView->prependNodeMenuAction(d->InsertHierarchyAction);
    }
  else
    {
    d->ModelHierarchyTreeView->removeNodeMenuAction(d->InsertHierarchyAction);
    }
}

//-----------------------------------------------------------------------------
void qSlicerModelsModuleWidget::onCollapsed(const QModelIndex & index)
{
  Q_D(qSlicerModelsModuleWidget);
//  qDebug() << "qSlicerModelsModuleWidget::onCollapsed";
  
  vtkMRMLNode *node = d->ModelHierarchyTreeView->sortFilterProxyModel()->mrmlNodeFromIndex(index);
  if (!node)
    {
    qDebug() << "qSlicerModelsModuleWidget::onCollapsed: node not found for this index";
    return;
    }
  vtkMRMLDisplayableHierarchyNode *hnode = vtkMRMLDisplayableHierarchyNode::SafeDownCast(node);
  if (!hnode)
    {
    qWarning() << "qSlicerModelsModuleWidget::onCollapsed: node is not a displayable hierarchy node";
    return;
    }
//  qDebug() << "qSlicerModelsModuleWidget::onCollapsed: found displayable hierarchy node "<< hnode->GetID();

  hnode->SetExpanded(0);
//  qDebug() << "qSlicerModelsModuleWidget::onCollapsed: set hierarchy node to collapsed";
}

//-----------------------------------------------------------------------------
void qSlicerModelsModuleWidget::onExpanded(const QModelIndex & index)
{
  Q_D(qSlicerModelsModuleWidget);
//  qDebug() << "qSlicerModelsModuleWidget::onExpanded";
  
  vtkMRMLNode *node = d->ModelHierarchyTreeView->sortFilterProxyModel()->mrmlNodeFromIndex(index);
  if (!node)
    {
    qWarning() << "qSlicerModelsModuleWidget::onExpanded: node not found for this index";
    return;
    }
  vtkMRMLDisplayableHierarchyNode *hnode = vtkMRMLDisplayableHierarchyNode::SafeDownCast(node);
  if (!hnode)
    {
    qWarning() << "qSlicerModelsModuleWidget::onExpanded: node is not a displayable hierarchy node";
    return;
    }
//  qDebug() << "qSlicerModelsModuleWidget::onExpanded: found displayable hierarchy node " << hnode->GetID();

  hnode->SetExpanded(1);
//  qDebug() << "qSlicerModelsModuleWidget::onExpanded: set hierarchy node to expanded";
}

//-----------------------------------------------------------------------------
void qSlicerModelsModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  //Q_D(qSlicerModelsModuleWidget);

  if (scene == this->mrmlScene())
    {
    return;
    }

  // listen for import end events
  qvtkReconnect(this->mrmlScene(), scene,
                vtkMRMLScene::EndImportEvent,
                this, SLOT(onEndImportEvent()));

  this->Superclass::setMRMLScene(scene);

}


//-----------------------------------------------------------------------------
void qSlicerModelsModuleWidget::onEndImportEvent()
{
  this->updateTreeViewModel();
  
//  qDebug() << "qSlicerModelsModuleWidget::onEndImportEvent: after updating tree view model";
}
