using Corium3D;
using System;
using Microsoft.WindowsAPICodePack.Dialogs;
using System.Collections.ObjectModel;
using System.Windows.Input;
using Corium3DGI.Utils;
using System.Windows.Media.Media3D;
using System.Windows;
using System.Windows.Controls;
using CoriumDirectX;
using System.Linq;
using System.Runtime.CompilerServices;

namespace Corium3DGI
{
    public class MainWindowVM : ObservableObject
    {
        private enum CameraAction { REST, ROTATE, PAN };

        private const double MODEL_ROT_PER_CURSOR_MOVE = 180 / 1000.0;
        private const double CAMERA_ZOOM_PER_MOUSE_WHEEL_TURN = 0.005;        
        private const double DOUBLE_EPSILON = 1E-6;
        private const double SCENE_MODEL_INSTANCE_INIT_DIST_FROM_CAMERA = 10.0;
        private const uint GRID_HALF_WIDHT = 500;
        private const uint GRID_HALF_HEIGHT = 500;

        private DxVisualizer dxVisualizer;
        private uint gridDxModelId;
        private CameraAction cameraAction = CameraAction.REST;
        private bool isModelTransforming = false;
        private Point prevMousePos;
        private SceneModelInstanceM draggedSceneModelInstance = null;

        public ObservableCollection<ModelM> ModelMs { get; } = new ObservableCollection<ModelM>();
        public ObservableCollection<SceneM> SceneMs { get; } = new ObservableCollection<SceneM>();

        public double CameraFieldOfView { get; } = 60;
        public double CameraNearPlaneDist { get; } = 0.01; //0.125;
        public double CameraFarPlaneDist { get; } = 1000;

        private ModelM selectedModel;
        public ModelM SelectedModel
        {
            get { return selectedModel; }

            set
            {
                if (selectedModel != value)
                {
                    selectedModel = value;
                    OnPropertyChanged("SelectedModel");
                }
            }
        }

        private SceneM selectedScene;
        public SceneM SelectedScene
        {
            get { return selectedScene; }

            set
            {
                if (selectedScene != value)
                {
                    selectedScene = value;
                    if (selectedScene != null)
                        selectedScene.IDxScene.activate();
                    OnPropertyChanged("SelectedScene");
                }
            }
        }

        private SceneModelM selectedSceneModel;
        public SceneModelM SelectedSceneModel
        {
            get { return selectedSceneModel; }

            set
            {
                if (selectedSceneModel != value)
                {
                    selectedSceneModel = value;
                    OnPropertyChanged("SelectedSceneModel");
                }
            }
        }

        private SceneModelInstanceM selectedSceneModelInstance;
        public SceneModelInstanceM SelectedSceneModelInstance
        {
            get { return selectedSceneModelInstance; }

            set
            {
                if (selectedSceneModelInstance != value)
                {
                    selectedSceneModelInstance = value;
                    OnPropertyChanged("SelectedSceneModelInstance");
                }
            }
        }

        private Model3DGroup sceneModelInstances;
        public Model3DGroup SceneModelInstances
        {
            get { return sceneModelInstances; }

            set
            {
                if (sceneModelInstances != value)
                {
                    sceneModelInstances = value;
                    OnPropertyChanged("SceneModelInstances");
                }
            }
        }

        private Matrix3D cameraTransformat = new Matrix3D();
        public Matrix3D CameraTransformat
        {
            get { return cameraTransformat; }

            set
            {
                if (cameraTransformat != value)
                {
                    cameraTransformat = value;
                    OnPropertyChanged("CameraTransformat");
                }
            }
        }

        private ObservablePoint3D cameraPos = new ObservablePoint3D(0, 0, 5);
        public ObservablePoint3D CameraPos
        {
            get { return cameraPos; }

            set
            {
                if (cameraPos != value)
                {
                    cameraPos = value;
                    OnPropertyChanged("CameraPos");
                }
            }
        }

        private Vector3D cameraLookDirection = new Vector3D(0, 0, -1);
        public Vector3D CameraLookDirection
        {
            get { return cameraLookDirection; }

            set
            {
                if (cameraLookDirection != value)
                {
                    cameraLookDirection = value;
                    OnPropertyChanged("CameraLookDirection");
                }
            }
        }

        private Vector3D cameraUpDirection = new Vector3D(0, 1, 0);
        public Vector3D CameraUpDirection
        {
            get { return cameraUpDirection; }

            set
            {
                if (cameraUpDirection != value)
                {
                    cameraUpDirection = value;
                    OnPropertyChanged("CameraUpDirection");
                }
            }
        }

        private double gridLineThickness;
        public double GridLineThickness
        {
            get { return gridLineThickness; }

            set
            {
                if (gridLineThickness != value)
                {
                    gridLineThickness = value;
                    OnPropertyChanged("GridLineThickness");
                }
            }
        }        


        private Point3D modelCameraPos = new Point3D(0, 0, 5);
        public Point3D ModelCameraPos
        {
            get { return modelCameraPos; }

            set
            {
                if (modelCameraPos != value)
                {
                    modelCameraPos = value;
                    OnPropertyChanged("ModelCameraPos");
                }
            }
        }

        public ICommand ImportModelCmd { get; }
        public ICommand RemoveModelCmd { get; }
        public ICommand SaveImportedModelsCmd { get; }
        public ICommand AddSceneCmd { get; }
        public ICommand RemoveSceneCmd { get; }
        public ICommand AddSceneModelCmd { get; }
        public ICommand RemoveSceneModelCmd { get; }
        public ICommand OnViewportLoadedCmd { get; }
        public ICommand OnViewportSizeChangedCmd { get; }        
        public ICommand TrySceneModelDragStartCmd { get; }
        public ICommand DragEnterSceneViewportCmd { get; }
        public ICommand DragLeaveSceneViewportCmd { get; }
        public ICommand DragOverSceneViewportCmd { get; }
        public ICommand DropSceneViewportCmd { get; }
        public ICommand AddSceneModelInstanceCmd { get; }
        public ICommand RemoveSceneModelInstanceCmd { get; }
        public ICommand SaveSceneCmd { get; }
        public ICommand MouseDownSceneViewportCmd { get; }
        public ICommand MouseUpSceneViewportCmd { get; }
        public ICommand MouseMoveSceneViewportCmd { get; }
        public ICommand MouseWheelSceneViewportCmd { get; }        
        public ICommand MouseDownModelViewportCmd { get; }
        public ICommand MouseUpModelViewportCmd { get; }
        public ICommand MouseMoveModelViewportCmd { get; }
        public ICommand MouseWheelModelViewportCmd { get; }
        public ICommand ClearFocusCmd { get; }

        public MainWindowVM(DxVisualizer dxVisualizer)
        {
            this.dxVisualizer = dxVisualizer;

            ImportModelCmd = new RelayCommand(p => importModel());
            RemoveModelCmd = new RelayCommand(p => removeModel(), p => SelectedModel != null);
            SaveImportedModelsCmd = new RelayCommand(p => saveImportedModels(), p => ModelMs.Count > 0);
            AddSceneCmd = new RelayCommand(p => addScene((KeyboardFocusChangedEventArgs)p));
            RemoveSceneCmd = new RelayCommand(p => removeScene());
            AddSceneModelCmd = new RelayCommand(p => addSceneModel(), p => SelectedModel != null && SelectedScene != null);
            RemoveSceneModelCmd = new RelayCommand(p => removeSceneModel(), p => SelectedSceneModel != null);
            OnViewportLoadedCmd = new RelayCommand(p => onViewportLoaded());
            OnViewportSizeChangedCmd = new RelayCommand(p => onViewportSizeChagned((SizeChangedEventArgs)p));
            TrySceneModelDragStartCmd = new RelayCommand(p => trySceneModelDragStart((MouseEventArgs)p));
            DragEnterSceneViewportCmd = new RelayCommand(p => sceneModelDragEnterViewport((DragEventArgs)p));
            DragLeaveSceneViewportCmd = new RelayCommand(p => sceneModelDragLeaveViewport((DragEventArgs)p));
            DragOverSceneViewportCmd = new RelayCommand(p => sceneModelDragOverViewport((DragEventArgs)p));
            DropSceneViewportCmd = new RelayCommand(p => sceneModelDropOnViewport((DragEventArgs)p));
            //AddSceneModelInstanceCmd = new RelayCommand(p => addSceneModelInstance(), p => SelectedSceneModel != null);
            RemoveSceneModelInstanceCmd = new RelayCommand(p => removeSceneModelInstance(), p => SelectedSceneModelInstance != null);
            SaveSceneCmd = new RelayCommand(p => saveScene(), p => SelectedScene != null);
            MouseDownSceneViewportCmd = new RelayCommand(p => mouseDownSceneViewport((MouseButtonEventArgs)p));
            MouseUpSceneViewportCmd = new RelayCommand(p => mouseUpSceneViewport((MouseButtonEventArgs)p));
            MouseMoveSceneViewportCmd = new RelayCommand(p => mouseMoveSceneViewport((MouseEventArgs)p));
            MouseWheelSceneViewportCmd = new RelayCommand(p => zoomCamera((MouseWheelEventArgs)p));
            MouseDownModelViewportCmd = new RelayCommand(p => modelRotateStart((MouseButtonEventArgs)p));
            MouseUpModelViewportCmd = new RelayCommand(p => modelRotateEnd((MouseButtonEventArgs)p));
            MouseMoveModelViewportCmd = new RelayCommand(p => rotateModel((MouseEventArgs)p));
            MouseWheelModelViewportCmd = new RelayCommand(p => zoomModelCamera((MouseWheelEventArgs)p));
            ClearFocusCmd = new RelayCommand(p => clearFocus());

            // Forcing a call to CollisionPrimitives' static constructors
            CollisionPrimitive collisionPrimitive = new CollisionPrimitive();
            CollisionBox collisionBox = new CollisionBox(new Point3D(), new Point3D());
            CollisionSphere collisionSphere = new CollisionSphere(new Point3D(), 0);
            CollisionCapsule collisionCapsule = new CollisionCapsule(new Point3D(), new Vector3D(), 0, 0);
            CollisionRect collisionRect = new CollisionRect(new Point(), new Point());
            CollisionCircle collisionCircle = new CollisionCircle(new Point(), 0);
            CollisionStadium collisionStadium = new CollisionStadium(new Point(), new Vector(), 0, 0);            
        }

        private void importModel()
        {
            Microsoft.Win32.OpenFileDialog dlg = new Microsoft.Win32.OpenFileDialog()
            {
                Title = "Import 3D Model",
                DefaultExt = ".dae",
                Filter = "Collada|*.dae;*.fbx",
                Multiselect = true
            };

            Nullable<bool> result = dlg.ShowDialog();
            if (result == true)
            {
                foreach (string modelColladaPath in dlg.FileNames)
                {
                    AssetsImporter.NamedModelData importedModelData = AssetsImporter.Instance.importModel(modelColladaPath);                    
                    uint dxModelID;
                    dxVisualizer.addModel(importedModelData.importData.meshesVertices[0], importedModelData.importData.meshesVertexIndices[0], PrimitiveTopology.TRIANGLELIST, out dxModelID);
                    ModelM model = new ModelM(importedModelData, dxModelID);
                    ModelMs.Add(model);
                }
            }
        }

        private void removeModel()
        {
            AssetsImporter.Instance.removeModel(SelectedModel.Name);
            dxVisualizer.removeModel(SelectedModel.DxModelID);
            foreach (SceneM scene in SceneMs)
                scene.removeSceneModel(SelectedModel);
            ModelMs.Remove(SelectedModel);
        }

        private void saveImportedModels()
        {
            CommonOpenFileDialog selectFolderDlg = new CommonOpenFileDialog()
            {
                IsFolderPicker = true
            };

            if (selectFolderDlg.ShowDialog() == CommonFileDialogResult.Ok)
            {
                foreach (ModelM model in ModelMs)
                {
                    switch (model.CollisionPrimitive3DSelected)
                    {
                        case CollisionBox cb:
                            AssetsImporter.Instance.assignCollisionBox(model.Name, cb.Center.Point3DCpy, cb.Scale.Point3DCpy);
                            break;
                        case CollisionSphere cs:
                            AssetsImporter.Instance.assignCollisionSphere(model.Name, cs.Center.Point3DCpy, cs.Radius);
                            break;
                        case CollisionCapsule cc:
                            Vector3D axisVec = cc.Height * cc.AxisVec.Vector3DCpy;
                            AssetsImporter.Instance.assignCollisionCapsule(model.Name, cc.Center.Point3DCpy - 0.5 * axisVec, axisVec, cc.Radius);
                            break;
                        default:
                            AssetsImporter.Instance.clearCollisionPrimitive3D(model.Name);
                            break;
                    }

                    switch (model.CollisionPrimitive2DSelected)
                    {
                        case CollisionRect cr:
                            AssetsImporter.Instance.assignCollisionRect(model.Name, cr.Center.PointCpy, cr.Scale.PointCpy);
                            break;
                        case CollisionCircle cc:
                            AssetsImporter.Instance.assignCollisionCircle(model.Name, cc.Center.PointCpy, cc.Radius);
                            break;
                        case CollisionStadium cs:
                            Vector axisVec = cs.Height * cs.AxisVec.VectorCpy;
                            AssetsImporter.Instance.assignCollisionStadium(model.Name, cs.Center.PointCpy - 0.5 * axisVec, axisVec, cs.Radius);
                            break;
                        default:
                            AssetsImporter.Instance.clearCollisionPrimitive2D(model.Name);
                            break;
                    }
                }

                AssetsImporter.Instance.saveImportedModels(selectFolderDlg.FileName);
            }
        }

        private void addScene(KeyboardFocusChangedEventArgs e)
        {
            SceneM addedSceneM = new SceneM(((TextBox)e.Source).Text, dxVisualizer.createScene());
            addedSceneM.IDxScene.createModelInstance(gridDxModelId, new Vector3D(0, 0, 0), new Vector3D(1, 1, 1), new Vector3D(1, 0, 0), 0, null);
            SceneMs.Add(addedSceneM);
            SelectedScene = addedSceneM;
        }

        private void removeScene()
        {
            SelectedScene.releaseDxLmnts();
            SceneMs.Remove(SelectedScene);                        
        }

        private void addSceneModel()
        {
            SelectedSceneModel = SelectedScene.addSceneModel(SelectedModel);            
        }

        private void removeSceneModel()
        {
            SelectedScene.removeSceneModel(SelectedSceneModel);            
        }

        private void onViewportLoaded()
        {
            uint vertGridLinesNr = 2 * GRID_HALF_WIDHT + 1;
            uint horizonGridLinesNr = 2 * GRID_HALF_HEIGHT + 1;
            Point3D[] vertices = new Point3D[2 * (vertGridLinesNr + horizonGridLinesNr)];
            ushort[] vertexIndices = new ushort[2 * (vertGridLinesNr + horizonGridLinesNr)];
            for (uint vertLineIdx = 0; vertLineIdx < vertGridLinesNr; vertLineIdx++)
            {
                vertices[2 * vertLineIdx] = new Point3D(-GRID_HALF_WIDHT + vertLineIdx, 0.0f, -GRID_HALF_HEIGHT);
                vertices[2 * vertLineIdx + 1] = new Point3D(-GRID_HALF_WIDHT + vertLineIdx, 0.0f, GRID_HALF_HEIGHT);
                vertexIndices[2 * vertLineIdx] = (ushort)(2 * vertLineIdx);
                vertexIndices[2 * vertLineIdx + 1] = (ushort)(2 * vertLineIdx + 1);
            }
            for (uint horizonLineIdx = 0; horizonLineIdx < horizonGridLinesNr; horizonLineIdx++)
            {
                vertices[2 * (vertGridLinesNr + horizonLineIdx)] = new Point3D(-GRID_HALF_WIDHT, 0.0f, -GRID_HALF_HEIGHT + horizonLineIdx);
                vertices[2 * (vertGridLinesNr + horizonLineIdx) + 1] = new Point3D(GRID_HALF_WIDHT, 0.0f, -GRID_HALF_HEIGHT + horizonLineIdx);
                vertexIndices[2 * (vertGridLinesNr + horizonLineIdx)] = (ushort)(2 * (vertGridLinesNr + horizonLineIdx));
                vertexIndices[2 * (vertGridLinesNr + horizonLineIdx) + 1] = (ushort)(2 * (vertGridLinesNr + horizonLineIdx) + 1);
            }

            dxVisualizer.addModel(vertices, vertexIndices, PrimitiveTopology.LINELIST, out gridDxModelId);
        }

        private void onViewportSizeChagned(SizeChangedEventArgs e)
        {
            
        }   

        private void trySceneModelDragStart(MouseEventArgs e)
        {
            if (e.Source is DependencyObject && SelectedSceneModel != null && e.LeftButton == MouseButtonState.Pressed)
                DragDrop.DoDragDrop((DependencyObject)e.Source, SelectedSceneModel, DragDropEffects.Copy);
        }

        private void sceneModelDragEnterViewport(DragEventArgs e)
        {            
            if (e.Data.GetDataPresent(typeof(SceneModelM)))
            {
                SelectedSceneModelInstance = draggedSceneModelInstance = SelectedSceneModel.addSceneModelInstance(
                    cursorPosToDraggedModelTranslation(e.GetPosition((FrameworkElement)e.OriginalSource)), new Vector3D(1, 1, 1), new Vector3D(1, 0, 0), 0, onSceneModelInstanceSelected);                                
            }
        }

        private void sceneModelDragLeaveViewport(DragEventArgs e)
        {
            if (draggedSceneModelInstance != null)
            {
                SelectedSceneModel.removeSceneModelInstance(draggedSceneModelInstance);
                SelectedSceneModelInstance = draggedSceneModelInstance = null;
            }
        }

        private void sceneModelDragOverViewport(DragEventArgs e)
        {            
            if (draggedSceneModelInstance != null)
            {                                
                Vector3D draggedSceneModelTranslation = cursorPosToDraggedModelTranslation(e.GetPosition((FrameworkElement)e.OriginalSource));
                draggedSceneModelInstance.Translate.X = draggedSceneModelTranslation.X;
                draggedSceneModelInstance.Translate.Y = draggedSceneModelTranslation.Y;
                draggedSceneModelInstance.Translate.Z = draggedSceneModelTranslation.Z;
            }
        }

        private Vector3D cursorPosToDraggedModelTranslation(Point cursorPos)
        {
            Vector3D rayDirection = (Vector3D)SelectedScene.IDxScene.cursorPosToRayDirection((float)cursorPos.X, (float)cursorPos.Y);
            return (Vector3D)SelectedScene.IDxScene.getCameraPos() + (SCENE_MODEL_INSTANCE_INIT_DIST_FROM_CAMERA * rayDirection);
        }

        private void sceneModelDropOnViewport(DragEventArgs e)
        {
            draggedSceneModelInstance = null;
        }

        private void removeSceneModelInstance()
        {
            SelectedSceneModel.removeSceneModelInstance(SelectedSceneModelInstance);            
        }

        private void saveScene()
        {            
            
        }

        private void mouseDownSceneViewport(MouseButtonEventArgs e)
        {
            if (e.LeftButton == MouseButtonState.Pressed)
            {
                Point mousePos = e.GetPosition((FrameworkElement)e.Source);
                if (!SelectedScene.cursorSelect((float)mousePos.X, (float)mousePos.Y))
                    SelectedSceneModelInstance = null;
            }
            else if (cameraAction == CameraAction.REST)
            {
                if (e.RightButton == MouseButtonState.Pressed)
                {
                    Mouse.Capture((UIElement)e.OriginalSource);
                    prevMousePos = e.GetPosition(Application.Current.MainWindow);
                    cameraAction = CameraAction.ROTATE;
                }
                else if (e.MiddleButton == MouseButtonState.Pressed)
                {
                    Mouse.Capture((UIElement)e.OriginalSource);
                    prevMousePos = e.GetPosition(Application.Current.MainWindow);
                    cameraAction = CameraAction.PAN;
                }
            }
        }

        private void onSceneModelInstanceSelected(SceneModelInstanceM selectedSceneModelInstance)
        {
            SelectedSceneModelInstance = selectedSceneModelInstance;
        }

        private void mouseUpSceneViewport(MouseButtonEventArgs e)
        {
            if (cameraAction == CameraAction.ROTATE && e.RightButton == MouseButtonState.Released || cameraAction == CameraAction.PAN && e.MiddleButton == MouseButtonState.Released)
            {
                cameraAction = CameraAction.REST;
                Mouse.Capture(null);
            }
        }

        private void mouseMoveSceneViewport(MouseEventArgs e)
        {
            if (cameraAction != CameraAction.REST)
            {
                Point currMousePos = e.GetPosition(Application.Current.MainWindow);
                Vector cursorMoveVec = currMousePos - prevMousePos;
                if (Math.Abs(cursorMoveVec.X) > DOUBLE_EPSILON || Math.Abs(cursorMoveVec.Y) > DOUBLE_EPSILON)
                {
                    if (cameraAction == CameraAction.ROTATE)
                        selectedScene.IDxScene.rotateCamera((float)cursorMoveVec.X, (float)cursorMoveVec.Y);
                    else if (cameraAction == CameraAction.PAN)
                        selectedScene.IDxScene.panCamera((float)cursorMoveVec.X, (float)cursorMoveVec.Y);

                    prevMousePos = currMousePos;
                }
            }
        }

        private void zoomCamera(MouseWheelEventArgs e)
        {
            if (cameraAction == CameraAction.REST)
                selectedScene.IDxScene.zoomCamera(e.Delta);
        }        

        private void modelRotateStart(MouseButtonEventArgs e)
        {
            if (!isModelTransforming && cameraAction == CameraAction.REST && e.RightButton == MouseButtonState.Pressed)
            {
                Mouse.Capture((UIElement)e.OriginalSource);
                prevMousePos = e.GetPosition(Application.Current.MainWindow);
                isModelTransforming = true;
            }
        }

        private void modelRotateEnd(MouseButtonEventArgs e)
        {
            if (e.RightButton == MouseButtonState.Released)
            {
                isModelTransforming = false;
                Mouse.Capture(null);
            }
        }

        private void rotateModel(MouseEventArgs e)
        {
            if (isModelTransforming)
            {
                Point currMousePos = e.GetPosition(Application.Current.MainWindow);
                Vector cursorMoveVec = (currMousePos - prevMousePos);
                if (Math.Abs(cursorMoveVec.X) > DOUBLE_EPSILON || Math.Abs(cursorMoveVec.Y) > DOUBLE_EPSILON)
                {
                    Quaternion rotationQuatX = new Quaternion(new Vector3D(1, 0, 0), cursorMoveVec.Y * MODEL_ROT_PER_CURSOR_MOVE);
                    Matrix3D rotMat = Matrix3D.Identity;
                    rotMat.RotatePrepend(SelectedModel.Rotation);
                    Quaternion rotationQuatModelY = new Quaternion(Vector3D.Multiply(new Vector3D(0, 1, 0), rotMat), cursorMoveVec.X * MODEL_ROT_PER_CURSOR_MOVE);
                    SelectedModel.Rotation = rotationQuatX * rotationQuatModelY * SelectedModel.Rotation;

                    prevMousePos = currMousePos;
                }
            }
        }

        private void zoomModelCamera(MouseWheelEventArgs e)
        {
            ModelCameraPos += CAMERA_ZOOM_PER_MOUSE_WHEEL_TURN * e.Delta * new Vector3D(0,0,-1);            
        }
        private void clearFocus()
        {
            Keyboard.ClearFocus();
        }
    }
}
 