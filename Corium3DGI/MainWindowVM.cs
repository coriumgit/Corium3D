using System;
using Microsoft.WindowsAPICodePack.Dialogs;
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Media3D;

using System.Windows.Controls;
using CoriumDirectX;
using System.Collections.Generic;
using Corium3DGI.Utils;
using System.ComponentModel;

namespace Corium3DGI
{
    public class MainWindowVM : ObservableObject
    {
        private const double SCENE_CAMERA_FOV = 60.0;
        private class SceneModelInstanceDxManipulator
        {           
            private enum Handle { TranslateX, TranslateY , TranslateZ, Translate1D,
                                  ScaleX, ScaleZ, ScaleY, Scale,
                                  TranslateXY, TranslateXZ, TranslateYZ, Translate2D,
                                  RotateX, RotateY, RotateZ, Rotate,
                                  None 
            };

            private const float EPSILON = 1E-3f;
            private const uint HANDLES_CIRCLES_SEGS_NR = 20;
            private const float TASL= 1.0f; // := Translation Arrow Shaft Length
            private const float TRANSLATION_ARROW_HEAD_HEIGHT_TO_TASL_RATIO = 0.125f;
            private const float TRANSLATION_ARROW_HEAD_RADIUS_TO_TASL_RATIO = 0.05f;             
            private const float SCALE_ARROW_SHAFT_LEN_TO_TASL_RATIO = 0.5f;
            // scale arrow head side computed below : scale arrow head is a cube whose surface plane is confined within translation arrow head's base
            private const float TRANSLATION_RECT_SIDE_TO_TASL_RATIO = 0.3f;
            private const float ROTATION_RING_RADIUS_TO_TASL_RATIO = 1.2f;
            // rotation ring envelope thickness computed below: equals to half the translation arrow head radius
            private const float TASL_IN_VISUAL_DEGS_TO_FOV_RATIO = 0.2f;
            
            private const float SCALE_ARROW_HEAD_HALF_SIDE = 0.5f * 1.41421356f * TRANSLATION_ARROW_HEAD_RADIUS_TO_TASL_RATIO * TASL;
            private const float ROTATION_RING_ENVELOPE_THICKNESS_RADIUS = 0.5f * TRANSLATION_ARROW_HEAD_RADIUS_TO_TASL_RATIO * TASL;

            private static readonly Point3D[] translateArrowShaftVertices = new Point3D[2];
            private static readonly Point3D[] scaleArrowShaftVertices = new Point3D[2];
            private static readonly ushort[] arrowsShaftVertexIndices = new ushort[2];
            private static readonly Point3D[] translationArrowHeadVertices = new Point3D[HANDLES_CIRCLES_SEGS_NR + 2];
            private static readonly ushort[] translationArrowHeadVertexIndices = new ushort[6 * HANDLES_CIRCLES_SEGS_NR];                       
            private static readonly Point3D[] scaleArrowHeadVertices = new Point3D[8];
            private static readonly ushort[] scaleArrowHeadVertexIndices = new ushort[36];
            private static readonly Point3D[] translationArrowEnvelopeVertices = new Point3D[2 * HANDLES_CIRCLES_SEGS_NR];            
            private static readonly Point3D[] scaleArrowEnvelopeVertices = new Point3D[2 * HANDLES_CIRCLES_SEGS_NR];
            private static readonly ushort[] arrowEnvelopesVertexIndices = new ushort[6 * HANDLES_CIRCLES_SEGS_NR];
            private static readonly Point3D[] translationRectVertices = new Point3D[4];
            private static readonly ushort[] translationRectVertexIndices = new ushort[12];
            private static readonly Point3D[] rotationRingVertices = new Point3D[HANDLES_CIRCLES_SEGS_NR];
            private static readonly ushort[] rotationRingVertexIndices = new ushort[2*HANDLES_CIRCLES_SEGS_NR];
            private static readonly Point3D[] rotationRingEnvelopeVertices = new Point3D[HANDLES_CIRCLES_SEGS_NR * HANDLES_CIRCLES_SEGS_NR];
            private static readonly ushort[] rotationRingEnvelopeVertexIndices = new ushort[6 * HANDLES_CIRCLES_SEGS_NR * HANDLES_CIRCLES_SEGS_NR];

            private static List<DxVisualizer> dxVisualizers = new List<DxVisualizer>();
            private static List<uint[]> dxHandlesModelIDs = new List<uint[]>();
            
            private DxVisualizer.IScene dxScene; 
            private DxVisualizer.IScene.ISceneModelInstance[] dxHandles = new DxVisualizer.IScene.ISceneModelInstance[27];
            private DxVisualizer.IScene.SelectionHandler[] dxHandlesOnSelectDelegates = new DxVisualizer.IScene.SelectionHandler[12];
            private SceneModelInstanceM boundInstance = null;

            private Handle activeHandle = Handle.None;
            private Vector3D activeTranslationAxis = new Vector3D();
            private Vector3D activeScaleAxis = new Vector3D();
            private Vector3D activeScaleHandleVec = new Vector3D();
            private Vector3D activeTranslationPlaneNormal = new Vector3D();
            private Point3D prevTransformPlaneDragPoint;

            private Vector3D activeRotationAxis = new Vector3D();
            private Vector3D instancePosToRotationStartPosVec = new Vector3D();
            private Point prevCursorPos;                        

            public ObservableVector3D Translate { get; }

            public bool IsActivated { 
                get  { return activeHandle != Handle.None; } 
            }

            static SceneModelInstanceDxManipulator()
            {
                //handles' circles cosines and sines
                const float handlesCirclesSegsArg = 2.0f * (float)Math.PI / HANDLES_CIRCLES_SEGS_NR;
                float[] handlesCirclesSegsCosines = new float[HANDLES_CIRCLES_SEGS_NR];
                float[] handlesCirclesSegsSines = new float[HANDLES_CIRCLES_SEGS_NR];
                for (uint segIdx = 0; segIdx < HANDLES_CIRCLES_SEGS_NR; segIdx++)
                {
                    handlesCirclesSegsCosines[segIdx] = (float)Math.Cos(segIdx * handlesCirclesSegsArg);
                    handlesCirclesSegsSines[segIdx] = (float)Math.Sin(segIdx * handlesCirclesSegsArg);
                }

                // arrow shafts
                translateArrowShaftVertices[0] = new Point3D();
                translateArrowShaftVertices[1] = new Point3D(TASL, 0.0f, 0.0f);

                scaleArrowShaftVertices[0] = new Point3D();
                scaleArrowShaftVertices[1] = new Point3D(SCALE_ARROW_SHAFT_LEN_TO_TASL_RATIO * TASL, 0.0f, 0.0f);
                
                arrowsShaftVertexIndices[0] = 0;
                arrowsShaftVertexIndices[1] = 1;                

                // translation arrow head        
                const float translationArrowHeadRadius = TRANSLATION_ARROW_HEAD_RADIUS_TO_TASL_RATIO * TASL;
                for (uint vertexIdx = 0; vertexIdx < HANDLES_CIRCLES_SEGS_NR; vertexIdx++)
                {
                    translationArrowHeadVertices[vertexIdx] = new Point3D(
                        TASL,
                        translationArrowHeadRadius * handlesCirclesSegsSines[vertexIdx],
                        translationArrowHeadRadius * handlesCirclesSegsCosines[vertexIdx]
                    );
                }
                translationArrowHeadVertices[HANDLES_CIRCLES_SEGS_NR] = new Point3D(TASL * (1 + TRANSLATION_ARROW_HEAD_HEIGHT_TO_TASL_RATIO), 0.0f, 0.0f);
                translationArrowHeadVertices[HANDLES_CIRCLES_SEGS_NR + 1] = new Point3D(TASL, 0.0f, 0.0f);

                for (ushort segIdx = 0; segIdx < HANDLES_CIRCLES_SEGS_NR; segIdx++)
                {
                    translationArrowHeadVertexIndices[6 * segIdx] = translationArrowHeadVertexIndices[6 * segIdx + 3] = (ushort)((segIdx + 1) % HANDLES_CIRCLES_SEGS_NR);
                    translationArrowHeadVertexIndices[6 * segIdx + 1] = translationArrowHeadVertexIndices[6 * segIdx + 4] = segIdx;
                    translationArrowHeadVertexIndices[6 * segIdx + 2] = (ushort)HANDLES_CIRCLES_SEGS_NR;
                    translationArrowHeadVertexIndices[6 * segIdx + 3] = segIdx;
                    translationArrowHeadVertexIndices[6 * segIdx + 4] = (ushort)((segIdx + 1) % HANDLES_CIRCLES_SEGS_NR);
                    translationArrowHeadVertexIndices[6 * segIdx + 5] = (ushort)HANDLES_CIRCLES_SEGS_NR + 1;
                }

                // scale arrow head                
                const float scaleArrowShaftLen = SCALE_ARROW_SHAFT_LEN_TO_TASL_RATIO * TASL;
                scaleArrowHeadVertices[0] = new Point3D(scaleArrowShaftLen, SCALE_ARROW_HEAD_HALF_SIDE, SCALE_ARROW_HEAD_HALF_SIDE);
                scaleArrowHeadVertices[1] = new Point3D(scaleArrowShaftLen, SCALE_ARROW_HEAD_HALF_SIDE, -SCALE_ARROW_HEAD_HALF_SIDE);
                scaleArrowHeadVertices[2] = new Point3D(scaleArrowShaftLen, -SCALE_ARROW_HEAD_HALF_SIDE, -SCALE_ARROW_HEAD_HALF_SIDE);
                scaleArrowHeadVertices[3] = new Point3D(scaleArrowShaftLen, -SCALE_ARROW_HEAD_HALF_SIDE, SCALE_ARROW_HEAD_HALF_SIDE);
                for (uint vertexIdx = 0; vertexIdx < 4; vertexIdx++)                
                    scaleArrowHeadVertices[4 + vertexIdx] = scaleArrowHeadVertices[vertexIdx] + new Vector3D(2.0f * SCALE_ARROW_HEAD_HALF_SIDE, 0.0f, 0.0f);

                //left face
                scaleArrowHeadVertexIndices[0] = 0; scaleArrowHeadVertexIndices[1] = 1; scaleArrowHeadVertexIndices[2] = 3;
                scaleArrowHeadVertexIndices[3] = 3; scaleArrowHeadVertexIndices[4] = 1; scaleArrowHeadVertexIndices[5] = 2;
                //top face
                scaleArrowHeadVertexIndices[6] = 0; scaleArrowHeadVertexIndices[7] = 4; scaleArrowHeadVertexIndices[8] = 1;
                scaleArrowHeadVertexIndices[9] = 1; scaleArrowHeadVertexIndices[10] = 4; scaleArrowHeadVertexIndices[11] = 5;
                //front face
                scaleArrowHeadVertexIndices[12] = 1; scaleArrowHeadVertexIndices[13] = 5; scaleArrowHeadVertexIndices[14] = 6;
                scaleArrowHeadVertexIndices[15] = 6; scaleArrowHeadVertexIndices[16] = 2; scaleArrowHeadVertexIndices[17] = 1;
                //right face
                scaleArrowHeadVertexIndices[18] = 5; scaleArrowHeadVertexIndices[19] = 4; scaleArrowHeadVertexIndices[20] = 7;
                scaleArrowHeadVertexIndices[21] = 7; scaleArrowHeadVertexIndices[22] = 6; scaleArrowHeadVertexIndices[23] = 5;
                //bottom face
                scaleArrowHeadVertexIndices[24] = 3; scaleArrowHeadVertexIndices[25] = 2; scaleArrowHeadVertexIndices[26] = 6;
                scaleArrowHeadVertexIndices[27] = 6; scaleArrowHeadVertexIndices[28] = 7; scaleArrowHeadVertexIndices[29] = 3;
                //back face
                scaleArrowHeadVertexIndices[30] = 0; scaleArrowHeadVertexIndices[31] = 3; scaleArrowHeadVertexIndices[32] = 7;
                scaleArrowHeadVertexIndices[33] = 7; scaleArrowHeadVertexIndices[34] = 4; scaleArrowHeadVertexIndices[35] = 0;

                // arrows envelopes
                float translationArrowEnvelopeLen = TASL * (1 + TRANSLATION_ARROW_HEAD_HEIGHT_TO_TASL_RATIO);
                float scaleArrowEnvelopeLen = scaleArrowShaftLen + 2.0f * SCALE_ARROW_HEAD_HALF_SIDE;
                for (uint vertexIdx = 0; vertexIdx < HANDLES_CIRCLES_SEGS_NR; vertexIdx++)
                {
                    translationArrowEnvelopeVertices[vertexIdx] = scaleArrowEnvelopeVertices[vertexIdx] = new Point3D(0.0f, 0.5f*translationArrowHeadVertices[vertexIdx].Y, 0.5f*translationArrowHeadVertices[vertexIdx].Z);
                    translationArrowEnvelopeVertices[vertexIdx + HANDLES_CIRCLES_SEGS_NR] = new Point3D(translationArrowEnvelopeLen, translationArrowHeadVertices[vertexIdx].Y, translationArrowHeadVertices[vertexIdx].Z);
                    scaleArrowEnvelopeVertices[vertexIdx + HANDLES_CIRCLES_SEGS_NR] = new Point3D(scaleArrowEnvelopeLen, translationArrowHeadVertices[vertexIdx].Y, translationArrowHeadVertices[vertexIdx].Z);
                }

                for (ushort segIdx = 0; segIdx < HANDLES_CIRCLES_SEGS_NR; segIdx++)
                {
                    arrowEnvelopesVertexIndices[6*segIdx]= segIdx;
                    arrowEnvelopesVertexIndices[6*segIdx + 1] = (ushort)(HANDLES_CIRCLES_SEGS_NR + segIdx);
                    arrowEnvelopesVertexIndices[6*segIdx + 2] = (ushort)((segIdx + 1) % HANDLES_CIRCLES_SEGS_NR);
                    arrowEnvelopesVertexIndices[6*segIdx + 3] = (ushort)(HANDLES_CIRCLES_SEGS_NR + segIdx);
                    arrowEnvelopesVertexIndices[6*segIdx + 4] = (ushort)(HANDLES_CIRCLES_SEGS_NR + ((segIdx + 1) % HANDLES_CIRCLES_SEGS_NR));
                    arrowEnvelopesVertexIndices[6*segIdx + 5] = (ushort)((segIdx + 1) % HANDLES_CIRCLES_SEGS_NR);
                }

                // translation rect
                const float translationRectSide = TRANSLATION_RECT_SIDE_TO_TASL_RATIO * TASL;
                translationRectVertices[0] = new Point3D();                
                translationRectVertices[1] = new Point3D(translationRectSide, 0.0f, 0.0f);
                translationRectVertices[2] = new Point3D(translationRectSide, translationRectSide, 0.0f);
                translationRectVertices[3] = new Point3D(0.0f, translationRectSide, 0.0f);

                translationRectVertexIndices[0] = 0; translationRectVertexIndices[1] = 3; translationRectVertexIndices[2] = 2;
                translationRectVertexIndices[3] = 2; translationRectVertexIndices[4] = 1; translationRectVertexIndices[5] = 0;
                translationRectVertexIndices[6] = 0; translationRectVertexIndices[7] = 1; translationRectVertexIndices[8] = 2;
                translationRectVertexIndices[9] = 2; translationRectVertexIndices[10] = 3; translationRectVertexIndices[11] = 0;
               
                // rotation ring
                const float rotationRingRadius = ROTATION_RING_RADIUS_TO_TASL_RATIO * TASL;
                for (uint vertexIdx = 0; vertexIdx < HANDLES_CIRCLES_SEGS_NR; vertexIdx++)
                {
                    rotationRingVertices[vertexIdx] = new Point3D(
                        rotationRingRadius * handlesCirclesSegsCosines[vertexIdx],
                        rotationRingRadius * handlesCirclesSegsSines[vertexIdx],
                        0.0f
                    );
                }

                for (ushort segIdx = 0; segIdx < HANDLES_CIRCLES_SEGS_NR; segIdx++)
                {
                    rotationRingVertexIndices[2 * segIdx] = segIdx;
                    rotationRingVertexIndices[2 * segIdx + 1] = (ushort)((segIdx + 1) % HANDLES_CIRCLES_SEGS_NR);                    
                }

                // rotation ring envelope                                
                for (uint vertexIdx = 0; vertexIdx < HANDLES_CIRCLES_SEGS_NR; vertexIdx++)
                    rotationRingEnvelopeVertices[vertexIdx] = new Point3D(ROTATION_RING_ENVELOPE_THICKNESS_RADIUS * handlesCirclesSegsCosines[vertexIdx] + rotationRingRadius, 0.0f, ROTATION_RING_ENVELOPE_THICKNESS_RADIUS * handlesCirclesSegsSines[vertexIdx]);
                for (uint segIdx = 1; segIdx < HANDLES_CIRCLES_SEGS_NR; segIdx++)
                {
                    for (uint vertexIdx = 0; vertexIdx < HANDLES_CIRCLES_SEGS_NR; vertexIdx++)
                    {
                        rotationRingEnvelopeVertices[segIdx*HANDLES_CIRCLES_SEGS_NR + vertexIdx] =
                            new Point3D(rotationRingEnvelopeVertices[vertexIdx].X * handlesCirclesSegsCosines[segIdx],
                                        rotationRingEnvelopeVertices[vertexIdx].X * handlesCirclesSegsSines[segIdx],
                                        rotationRingEnvelopeVertices[vertexIdx].Z);
                    }
                }
                 
                for (uint segIdx = 0; segIdx < HANDLES_CIRCLES_SEGS_NR - 1; segIdx++)
                {
                    uint facesBaseIdx = 6 * segIdx * HANDLES_CIRCLES_SEGS_NR;
                    ushort vertexBaseIdx = (ushort)(segIdx * HANDLES_CIRCLES_SEGS_NR);
                    for (ushort faceIdx = 0; faceIdx < HANDLES_CIRCLES_SEGS_NR; faceIdx++)
                    {
                        rotationRingEnvelopeVertexIndices[facesBaseIdx + 6*faceIdx] = (ushort)(vertexBaseIdx + faceIdx);
                        rotationRingEnvelopeVertexIndices[facesBaseIdx + 6*faceIdx + 1] = (ushort)(vertexBaseIdx + HANDLES_CIRCLES_SEGS_NR + faceIdx);
                        rotationRingEnvelopeVertexIndices[facesBaseIdx + 6*faceIdx + 2] = (ushort)(vertexBaseIdx + (faceIdx + 1) % HANDLES_CIRCLES_SEGS_NR);
                        rotationRingEnvelopeVertexIndices[facesBaseIdx + 6*faceIdx + 3] = (ushort)(vertexBaseIdx + HANDLES_CIRCLES_SEGS_NR + faceIdx);
                        rotationRingEnvelopeVertexIndices[facesBaseIdx + 6*faceIdx + 4] = (ushort)(vertexBaseIdx + HANDLES_CIRCLES_SEGS_NR + (faceIdx + 1) % HANDLES_CIRCLES_SEGS_NR);
                        rotationRingEnvelopeVertexIndices[facesBaseIdx + 6*faceIdx + 5] = (ushort)(vertexBaseIdx + (faceIdx + 1) % HANDLES_CIRCLES_SEGS_NR);
                    }
                }
                
                {
                    ushort facesBaseIdx = (ushort)(6 * (HANDLES_CIRCLES_SEGS_NR - 1) * HANDLES_CIRCLES_SEGS_NR);
                    ushort vertexBaseIdx = (ushort)((HANDLES_CIRCLES_SEGS_NR - 1) * HANDLES_CIRCLES_SEGS_NR);
                    for (ushort faceIdx = 0; faceIdx < HANDLES_CIRCLES_SEGS_NR; faceIdx++)
                    {
                        rotationRingEnvelopeVertexIndices[facesBaseIdx + 6*faceIdx] = (ushort)(vertexBaseIdx + faceIdx);
                        rotationRingEnvelopeVertexIndices[facesBaseIdx + 6*faceIdx + 1] = faceIdx;
                        rotationRingEnvelopeVertexIndices[facesBaseIdx + 6*faceIdx + 2] = (ushort)(vertexBaseIdx + (faceIdx + 1) % HANDLES_CIRCLES_SEGS_NR);
                        rotationRingEnvelopeVertexIndices[facesBaseIdx + 6*faceIdx + 3] = faceIdx;
                        rotationRingEnvelopeVertexIndices[facesBaseIdx + 6*faceIdx + 4] = (ushort)((faceIdx + 1) % HANDLES_CIRCLES_SEGS_NR); 
                        rotationRingEnvelopeVertexIndices[facesBaseIdx + 6*faceIdx + 5] = (ushort)(vertexBaseIdx + (faceIdx + 1) % HANDLES_CIRCLES_SEGS_NR);
                    }
                }
                
            }

            public SceneModelInstanceDxManipulator(DxVisualizer dxVisualizer, DxVisualizer.IScene dxScene)
            {                
                this.dxScene = dxScene;

                uint[] handlesModelIDs;
                int dxVisualizerIdx = dxVisualizers.IndexOf(dxVisualizer);
                if (dxVisualizerIdx == -1)
                {
                    // handles Models IDs:
                    // handlesModelIDs[0] -> translation arrow shaft
                    // handlesModelIDs[1] -> translation arrow head
                    // handlesModelIDs[2] -> translation arrow envelope
                    // handlesModelIDs[3] -> scale arrow shaft
                    // handlesModelIDs[4] -> scale arrow head
                    // handlesModelIDs[5] -> scale arrow envelope
                    // handlesModelIDs[6] -> translation rect
                    // handlesModelIDs[7] -> rotation ring
                    // handlesModelIDs[8] -> rotation ring envelope
                    handlesModelIDs = new uint[9];
                    
                    const float translationArrowShaftHalfLen = 0.5f * TASL;
                    dxVisualizer.addModel(translateArrowShaftVertices, arrowsShaftVertexIndices, Color.FromArgb(255, 255, 0, 0),
                        new Point3D(translationArrowShaftHalfLen, 0.0f, 0.0f), translationArrowShaftHalfLen,
                        PrimitiveTopology.LINELIST, false, out handlesModelIDs[0]);
                
                    dxVisualizer.addModel(translationArrowHeadVertices, translationArrowHeadVertexIndices, Color.FromArgb(255, 0, 255, 0),
                        new Point3D(TASL, 0.0f, 0.0f), 0.5f * TASL * TRANSLATION_ARROW_HEAD_HEIGHT_TO_TASL_RATIO,
                        PrimitiveTopology.TRIANGLELIST, false, out handlesModelIDs[1]);
                    
                    const float translationArrowEnvelopeHalfLen = 0.5f * TASL * (1 + TRANSLATION_ARROW_HEAD_HEIGHT_TO_TASL_RATIO);
                    dxVisualizer.addModel(translationArrowEnvelopeVertices, arrowEnvelopesVertexIndices, Color.FromArgb(0, 255, 255, 255),
                        new Point3D(translationArrowEnvelopeHalfLen, 0.0f, 0.0f), translationArrowEnvelopeHalfLen,
                        PrimitiveTopology.TRIANGLELIST, false, out handlesModelIDs[2]);

                    const float scaleArrowShaftHalfLen = 0.5f * SCALE_ARROW_SHAFT_LEN_TO_TASL_RATIO * TASL;
                    dxVisualizer.addModel(scaleArrowShaftVertices, arrowsShaftVertexIndices, Color.FromArgb(255, 255, 0, 0),
                        new Point3D(scaleArrowShaftHalfLen, 0.0f, 0.0f), scaleArrowShaftHalfLen,
                        PrimitiveTopology.LINELIST, false, out handlesModelIDs[3]);

                    dxVisualizer.addModel(scaleArrowHeadVertices, scaleArrowHeadVertexIndices, Color.FromArgb(255, 0, 0, 255),
                        new Point3D(TASL + SCALE_ARROW_HEAD_HALF_SIDE, 0.0f, 0.0f), SCALE_ARROW_HEAD_HALF_SIDE,
                        PrimitiveTopology.TRIANGLELIST, false, out handlesModelIDs[4]);                    

                    const float scaleArrowEnvelopeHalfLen = 0.5f * (TASL + 2.0f * SCALE_ARROW_HEAD_HALF_SIDE);
                    dxVisualizer.addModel(scaleArrowEnvelopeVertices, arrowEnvelopesVertexIndices, Color.FromArgb(0, 255, 255, 255),
                        new Point3D(scaleArrowEnvelopeHalfLen, 0.0f, 0.0f), scaleArrowEnvelopeHalfLen,
                        PrimitiveTopology.TRIANGLELIST, false, out handlesModelIDs[5]);

                    
                    const float translationRectHalfSide = 0.5f * TRANSLATION_RECT_SIDE_TO_TASL_RATIO * TASL;                    
                    dxVisualizer.addModel(translationRectVertices, translationRectVertexIndices, Color.FromArgb(255, 255, 0, 255),
                        new Point3D(translationRectHalfSide, translationRectHalfSide, 0.0f), 1.41421356f * translationRectHalfSide,
                        PrimitiveTopology.TRIANGLELIST, false, out handlesModelIDs[6]);
                    
                    const float rotationRingRadius = ROTATION_RING_RADIUS_TO_TASL_RATIO * TASL;
                    dxVisualizer.addModel(rotationRingVertices, rotationRingVertexIndices, Color.FromArgb(255, 0, 255, 255),
                        new Point3D(), rotationRingRadius,
                        PrimitiveTopology.LINELIST, false, out handlesModelIDs[7]);

                    const float translationArrowHeadRadius = TRANSLATION_ARROW_HEAD_RADIUS_TO_TASL_RATIO * TASL;
                    dxVisualizer.addModel(rotationRingEnvelopeVertices, rotationRingEnvelopeVertexIndices, Color.FromArgb(0, 255, 255, 255),
                        new Point3D(), rotationRingRadius + translationArrowHeadRadius,
                        PrimitiveTopology.TRIANGLELIST, false, out handlesModelIDs[8]);
                    
                    dxHandlesModelIDs.Add(handlesModelIDs);
                    dxVisualizers.Add(dxVisualizer);
                }
                else                                    
                    handlesModelIDs = dxHandlesModelIDs[dxVisualizerIdx];

                dxHandlesOnSelectDelegates[0] = new DxVisualizer.IScene.SelectionHandler(activateTranslateOnX);
                dxHandlesOnSelectDelegates[1] = new DxVisualizer.IScene.SelectionHandler(activateTranslateOnY);
                dxHandlesOnSelectDelegates[2] = new DxVisualizer.IScene.SelectionHandler(activateTranslateOnZ);                
                dxHandlesOnSelectDelegates[3] = new DxVisualizer.IScene.SelectionHandler(activateScaleOnX);
                dxHandlesOnSelectDelegates[4] = new DxVisualizer.IScene.SelectionHandler(activateScaleOnY);
                dxHandlesOnSelectDelegates[5] = new DxVisualizer.IScene.SelectionHandler(activateScaleOnZ);                
                dxHandlesOnSelectDelegates[6] = new DxVisualizer.IScene.SelectionHandler(activateTranslateOnXY);
                dxHandlesOnSelectDelegates[7] = new DxVisualizer.IScene.SelectionHandler(activateTranslateOnXZ);
                dxHandlesOnSelectDelegates[8] = new DxVisualizer.IScene.SelectionHandler(activateTranslateOnYZ);                
                dxHandlesOnSelectDelegates[9] = new DxVisualizer.IScene.SelectionHandler(activateRotateRoundX);
                dxHandlesOnSelectDelegates[10] = new DxVisualizer.IScene.SelectionHandler(activateRotateRoundY);
                dxHandlesOnSelectDelegates[11] = new DxVisualizer.IScene.SelectionHandler(activateRotateRoundZ);                
                
                dxHandles[0] = dxScene.createModelInstance(handlesModelIDs[0], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(1.0f, 0.0f, 0.0f), 0.0f, dxHandlesOnSelectDelegates[0]);
                dxHandles[1] = dxScene.createModelInstance(handlesModelIDs[1], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(1.0f, 0.0f, 0.0f), 0.0f, dxHandlesOnSelectDelegates[0]);
                dxHandles[2] = dxScene.createModelInstance(handlesModelIDs[2], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(1.0f, 0.0f, 0.0f), 0.0f, dxHandlesOnSelectDelegates[0]);
                dxHandles[3] = dxScene.createModelInstance(handlesModelIDs[0], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(0.0f, 0.0f, 1.0f), 90.0f, dxHandlesOnSelectDelegates[1]);
                dxHandles[4] = dxScene.createModelInstance(handlesModelIDs[1], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(0.0f, 0.0f, 1.0f), 90.0f, dxHandlesOnSelectDelegates[1]);
                dxHandles[5] = dxScene.createModelInstance(handlesModelIDs[2], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(0.0f, 0.0f, 1.0f), 90.0f, dxHandlesOnSelectDelegates[1]);
                dxHandles[6] = dxScene.createModelInstance(handlesModelIDs[0], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(0.0f, 1.0f, 0.0f), -90.0f, dxHandlesOnSelectDelegates[2]);
                dxHandles[7] = dxScene.createModelInstance(handlesModelIDs[1], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(0.0f, 1.0f, 0.0f), -90.0f, dxHandlesOnSelectDelegates[2]);
                dxHandles[8] = dxScene.createModelInstance(handlesModelIDs[2], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(0.0f, 1.0f, 0.0f), -90.0f, dxHandlesOnSelectDelegates[2]);
                
                dxHandles[9] = dxScene.createModelInstance(handlesModelIDs[3], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(1.0f, 0.0f, 0.0f), 0.0f, dxHandlesOnSelectDelegates[3]);
                dxHandles[10] = dxScene.createModelInstance(handlesModelIDs[4], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(1.0f, 0.0f, 0.0f), 0.0f, dxHandlesOnSelectDelegates[3]);
                dxHandles[11] = dxScene.createModelInstance(handlesModelIDs[5], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(1.0f, 0.0f, 0.0f), 0.0f, dxHandlesOnSelectDelegates[3]);
                dxHandles[12] = dxScene.createModelInstance(handlesModelIDs[3], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(0.0f, 0.0f, 1.0f), 90.0f, dxHandlesOnSelectDelegates[4]);
                dxHandles[13] = dxScene.createModelInstance(handlesModelIDs[4], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(0.0f, 0.0f, 1.0f), 90.0f, dxHandlesOnSelectDelegates[4]);
                dxHandles[14] = dxScene.createModelInstance(handlesModelIDs[5], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(0.0f, 0.0f, 1.0f), 90.0f, dxHandlesOnSelectDelegates[4]);
                dxHandles[15] = dxScene.createModelInstance(handlesModelIDs[3], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(0.0f, 1.0f, 0.0f), -90.0f, dxHandlesOnSelectDelegates[5]);
                dxHandles[16] = dxScene.createModelInstance(handlesModelIDs[4], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(0.0f, 1.0f, 0.0f), -90.0f, dxHandlesOnSelectDelegates[5]);
                dxHandles[17] = dxScene.createModelInstance(handlesModelIDs[5], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(0.0f, 1.0f, 0.0f), -90.0f, dxHandlesOnSelectDelegates[5]);
                
                dxHandles[18] = dxScene.createModelInstance(handlesModelIDs[6], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(1.0f, 0.0f, 0.0f), 0.0f, dxHandlesOnSelectDelegates[6]);
                dxHandles[19] = dxScene.createModelInstance(handlesModelIDs[6], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(1.0f, 0.0f, 0.0f), 90.0f, dxHandlesOnSelectDelegates[7]);
                dxHandles[20] = dxScene.createModelInstance(handlesModelIDs[6], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(0.0f, 1.0f, 0.0f), -90.0f, dxHandlesOnSelectDelegates[8]);
                
                dxHandles[21] = dxScene.createModelInstance(handlesModelIDs[7], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(1.0f, 0.0f, 0.0f), 0.0f, dxHandlesOnSelectDelegates[11]);
                dxHandles[22] = dxScene.createModelInstance(handlesModelIDs[8], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(1.0f, 0.0f, 0.0f), 0.0f, dxHandlesOnSelectDelegates[11]);
                dxHandles[23] = dxScene.createModelInstance(handlesModelIDs[7], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(1.0f, 0.0f, 0.0f), 90.0f, dxHandlesOnSelectDelegates[10]);
                dxHandles[24] = dxScene.createModelInstance(handlesModelIDs[8], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(1.0f, 0.0f, 0.0f), 90.0f, dxHandlesOnSelectDelegates[10]);
                dxHandles[25] = dxScene.createModelInstance(handlesModelIDs[7], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(0.0f, 1.0f, 0.0f), -90.0f, dxHandlesOnSelectDelegates[9]);
                dxHandles[26] = dxScene.createModelInstance(handlesModelIDs[8], new Vector3D(), new Vector3D(1.0f, 1.0f, 1.0f), new Vector3D(0.0f, 1.0f, 0.0f), -90.0f, dxHandlesOnSelectDelegates[9]);                

                foreach (DxVisualizer.IScene.ISceneModelInstance handle in dxHandles)
                    handle.hide();
            }

            public void releaseDxLmnts()
            {
                foreach (DxVisualizer.IScene.ISceneModelInstance handle in dxHandles)
                    handle.release();
            }

            public void bindSceneModelInstance(SceneModelInstanceM sceneModelInstance)
            {
                if (boundInstance != null)
                {
                    boundInstance.TranslationSet -= onBoundInstanceTranslationSet;
                    boundInstance.RotSet -= onBoundInstanceRotate;
                }

                boundInstance = sceneModelInstance;
                boundInstance.TranslationSet += onBoundInstanceTranslationSet;
                boundInstance.RotSet += onBoundInstanceRotate;
                foreach (DxVisualizer.IScene.ISceneModelInstance handle in dxHandles)
                {
                    handle.show();
                    handle.setTranslation(boundInstance.Translate.Vector3DCpy);
                    updateDxHandlesScale();
                }
            }

            public void unbind()
            {
                boundInstance = null;
                foreach (DxVisualizer.IScene.ISceneModelInstance handle in dxHandles)                
                    handle.hide();
            }

            public void onMouseMove(Point cursorPos)
            {
                // manipulate the instance according to the handle activated in the direction of cursorMoveVec in world space (should make a method in IDxScene) relative to the handle
                if (activeHandle == Handle.None)
                    return;
                else
                {                    
                    if (activeHandle == Handle.TranslateX || activeHandle == Handle.TranslateY || activeHandle == Handle.TranslateZ)
                    {
                        Point3D currTransformPlaneDragPoint = transformAxisContainingPlaneDragPoint(activeTranslationAxis, (float)cursorPos.X, (float)cursorPos.Y);
                        Vector3D translationVec = Vector3D.DotProduct(activeTranslationAxis, currTransformPlaneDragPoint - prevTransformPlaneDragPoint) * activeTranslationAxis;
                        boundInstance.setTranslation(boundInstance.Translate.Vector3DCpy + translationVec);

                        prevTransformPlaneDragPoint = currTransformPlaneDragPoint;
                    }
                    else if (activeHandle == Handle.ScaleX || activeHandle == Handle.ScaleY || activeHandle == Handle.ScaleZ)
                    {
                        Console.WriteLine(activeScaleHandleVec);
                        Point3D currTransformPlaneDragPoint = transformAxisContainingPlaneDragPoint(activeScaleHandleVec, (float)cursorPos.X, (float)cursorPos.Y);
                        Vector3D scaleVec = Vector3D.DotProduct(activeScaleHandleVec, currTransformPlaneDragPoint - prevTransformPlaneDragPoint) * activeScaleAxis;
                        boundInstance.setScale(boundInstance.Scale.Vector3DCpy + scaleVec);

                        prevTransformPlaneDragPoint = currTransformPlaneDragPoint;
                    }
                    else if (activeHandle == Handle.TranslateXY || activeHandle == Handle.TranslateXZ || activeHandle == Handle.TranslateYZ)
                    {
                        Point3D currTransformPlaneDragPoint = translationPlaneDragPoint(activeTranslationPlaneNormal, (float)cursorPos.X, (float)cursorPos.Y);
                        Vector3D translationVec = currTransformPlaneDragPoint - prevTransformPlaneDragPoint;
                        boundInstance.setTranslation(boundInstance.Translate.Vector3DCpy + translationVec);

                        prevTransformPlaneDragPoint = currTransformPlaneDragPoint;
                    }
                    else // activeHandle == Handle.Rotate
                    {
                        Vector cursorMoveVec = cursorPos - prevCursorPos;
                        Vector3D cursorMoveVecInWorldSpace = (Vector3D)dxScene.screenVecToWorldVec(0.1f * (float)cursorMoveVec.X, 0.1f * (float)-cursorMoveVec.Y);
                        Quaternion quat = new Quaternion(activeRotationAxis, (float)Vector3D.DotProduct(Vector3D.CrossProduct(instancePosToRotationStartPosVec, cursorMoveVecInWorldSpace), activeRotationAxis));
                        boundInstance.setRotation(quat * boundInstance.Rot.QuaternionCpy);                        

                        prevCursorPos = cursorPos;
                    }
                }                                
            }

            public void onCameraTranslation()
            {
                updateDxHandlesScale();
            }

            public void deactivate()
            {
                activeHandle = Handle.None;
            }
            
            // doesnt check for lineVec (dot) planeNormal == 0
            private static Point3D linePlaneIntersection(Vector3D lineVec, Point3D linePoint, Vector3D planeNormal, Point3D planePoint)
            {
                return linePoint + lineVec*Vector3D.DotProduct(planePoint - linePoint, planeNormal) / Vector3D.DotProduct(lineVec, planeNormal);
            }

            private void onBoundInstanceTranslationSet(Vector3D translation)
            {
                setDxHandlesTranslation(translation);
                updateDxHandlesScale();
            }

            private void onBoundInstanceRotate(Quaternion rot)
            {                
                // rotate scaling handles                 
                for (uint dxHandleIdx = 9; dxHandleIdx <= 11; dxHandleIdx++)
                    dxHandles[dxHandleIdx].setRotation(boundInstance.Rot.Axis.Vector3DCpy, (float)boundInstance.Rot.Angle);

                Quaternion scaleYHandleRot = boundInstance.Rot.QuaternionCpy * new Quaternion(new Vector3D(0.0f, 0.0f, 1.0f), 90.0);
                for (uint dxHandleIdx = 12; dxHandleIdx <= 14; dxHandleIdx++)
                    dxHandles[dxHandleIdx].setRotation(scaleYHandleRot.Axis, (float)scaleYHandleRot.Angle);
                Quaternion scaleZHandleRot = boundInstance.Rot.QuaternionCpy * new Quaternion(new Vector3D(0.0f, 1.0f, 0.0f), -90.0);
                for (uint dxHandleIdx = 15; dxHandleIdx <= 17; dxHandleIdx++)
                    dxHandles[dxHandleIdx].setRotation(scaleZHandleRot.Axis, (float)scaleZHandleRot.Angle);
            }

            private Point3D transformAxisContainingPlaneDragPoint(Vector3D transformAxis, float cursorPosX, float cursorPosY)
            {
                Point3D cameraPos = (Point3D)dxScene.getCameraPos();
                Point3D boundInstancePos = (Point3D)boundInstance.Translate.Vector3DCpy;
                Vector3D containingPlaneNormal = Vector3D.CrossProduct(Vector3D.CrossProduct(transformAxis, cameraPos - boundInstancePos), transformAxis);

                return linePlaneIntersection((Vector3D)dxScene.cursorPosToRayDirection(cursorPosX, cursorPosY), cameraPos, containingPlaneNormal, boundInstancePos);
            }

            private void activateTranslateOnX(float x, float y)
            {
                activeHandle = Handle.TranslateX;
                activeTranslationAxis = new Vector3D(1.0f, 0.0f, 0.0f);
                prevTransformPlaneDragPoint = transformAxisContainingPlaneDragPoint(activeTranslationAxis, x, y);
            }

            private void activateTranslateOnY(float x, float y)
            {
                activeHandle = Handle.TranslateY;
                activeTranslationAxis = new Vector3D(0.0f, 1.0f, 0.0f);
                prevTransformPlaneDragPoint = transformAxisContainingPlaneDragPoint(activeTranslationAxis, x, y);
            }

            private void activateTranslateOnZ(float x, float y)
            {
                activeHandle = Handle.TranslateZ;
                activeTranslationAxis = new Vector3D(0.0f, 0.0f, 1.0f);
                prevTransformPlaneDragPoint = transformAxisContainingPlaneDragPoint(activeTranslationAxis, x, y);
            }
                         
            private void updateActiveScaleHandleVec()
            {                 
                Matrix3D rotMat = Matrix3D.Identity;
                rotMat.Rotate(boundInstance.Rot.QuaternionCpy);
                activeScaleHandleVec = rotMat.Transform(activeScaleAxis);
            }

            private void activateScaleOnX(float x, float y)
            {
                activeHandle = Handle.ScaleX;
                activeScaleAxis = new Vector3D(1.0f, 0.0f, 0.0f);
                updateActiveScaleHandleVec();
                prevTransformPlaneDragPoint = transformAxisContainingPlaneDragPoint(activeScaleHandleVec, x, y);
            }

            private void activateScaleOnY(float x, float y)
            {
                activeHandle = Handle.ScaleY;
                activeScaleAxis = new Vector3D(0.0f, 1.0f, 0.0f);
                updateActiveScaleHandleVec();
                prevTransformPlaneDragPoint = transformAxisContainingPlaneDragPoint(activeScaleHandleVec, x, y);
            }

            private void activateScaleOnZ(float x, float y)
            {
                activeHandle = Handle.ScaleZ;
                activeScaleAxis = new Vector3D(0.0f, 0.0f, 1.0f);
                updateActiveScaleHandleVec();
                prevTransformPlaneDragPoint = transformAxisContainingPlaneDragPoint(activeScaleHandleVec, x, y);
            }

            private Point3D translationPlaneDragPoint(Vector3D planeNormal, float cursorPosX, float cursorPosY)
            {
                return linePlaneIntersection((Vector3D)dxScene.cursorPosToRayDirection(cursorPosX, cursorPosY), (Point3D)dxScene.getCameraPos(),
                                             planeNormal, (Point3D)boundInstance.Translate.Vector3DCpy);
            }

            private void activateTranslateOnXY(float x, float y)
            {
                activeHandle = Handle.TranslateXY;
                activeTranslationPlaneNormal = new Vector3D(0.0f, 0.0f, 1.0f);
                prevTransformPlaneDragPoint = translationPlaneDragPoint(activeTranslationPlaneNormal, x, y);
            }

            private void activateTranslateOnXZ(float x, float y)
            {
                activeHandle = Handle.TranslateXZ;
                activeTranslationPlaneNormal = new Vector3D(0.0f, 1.0f, 0.0f);
                prevTransformPlaneDragPoint = translationPlaneDragPoint(activeTranslationPlaneNormal, x, y);
            }

            private void activateTranslateOnYZ(float x, float y)
            {
                activeHandle = Handle.TranslateYZ;
                activeTranslationPlaneNormal = new Vector3D(1.0f, 0.0f, 0.0f);
                prevTransformPlaneDragPoint = translationPlaneDragPoint(activeTranslationPlaneNormal, x, y);
            }

            private void activateRotateRoundX(float x, float y)
            {                
                activeHandle = Handle.RotateX;
                activeRotationAxis = new Vector3D(1.0f, 0.0f, 0.0f);

                Vector3D rayDirection = (Vector3D)dxScene.cursorPosToRayDirection(x, y);                
                Point3D cameraPos = (Point3D)dxScene.getCameraPos();
                Point3D boundInstancePos = (Point3D)boundInstance.Translate.Vector3DCpy;
                Point3D rotationStartPos = new Point3D(boundInstancePos.X, 0.0f, 0.0f);                
                if (Math.Abs(rayDirection.X) > EPSILON)
                {                    
                    float t = (float)((boundInstancePos.X - cameraPos.X) / rayDirection.X);
                    rotationStartPos.Y = cameraPos.Y + t*rayDirection.Y;
                    rotationStartPos.Z = cameraPos.Z + t*rayDirection.Z;
                    instancePosToRotationStartPosVec = rotationStartPos - boundInstancePos;
                }
                else
                    instancePosToRotationStartPosVec = cameraPos - boundInstancePos;

                instancePosToRotationStartPosVec.Normalize();

                prevCursorPos = new Point(x, y);
            }

            private void activateRotateRoundY(float x, float y)
            {
                activeHandle = Handle.RotateY;
                activeRotationAxis = new Vector3D(0.0f, 1.0f, 0.0f);

                Vector3D rayDirection = (Vector3D)dxScene.cursorPosToRayDirection(x, y);
                Point3D cameraPos = (Point3D)dxScene.getCameraPos();
                Point3D boundInstancePos = (Point3D)boundInstance.Translate.Vector3DCpy;
                Point3D rotationStartPos = new Point3D(0.0f, boundInstancePos.Y, 0.0f);
                if (Math.Abs(rayDirection.Y) > EPSILON)
                {
                    float t = (float)((boundInstancePos.Y - cameraPos.Y) / rayDirection.Y);
                    rotationStartPos.X = cameraPos.X + t * rayDirection.X;
                    rotationStartPos.Z = cameraPos.Z + t * rayDirection.Z;
                    instancePosToRotationStartPosVec = rotationStartPos - boundInstancePos;
                }
                else
                    instancePosToRotationStartPosVec = cameraPos - boundInstancePos;

                instancePosToRotationStartPosVec.Normalize();

                prevCursorPos = new Point(x, y);
            }

            private void activateRotateRoundZ(float x, float y)
            {
                activeHandle = Handle.RotateZ;
                activeRotationAxis = new Vector3D(0.0f, 0.0f, 1.0f);

                Vector3D rayDirection = (Vector3D)dxScene.cursorPosToRayDirection(x, y);
                Point3D cameraPos = (Point3D)dxScene.getCameraPos();
                Point3D boundInstancePos = (Point3D)boundInstance.Translate.Vector3DCpy;
                Point3D rotationStartPos = new Point3D(0.0f, 0.0f, boundInstancePos.Z);
                if (Math.Abs(rayDirection.Z) > EPSILON)
                {
                    float t = (float)((boundInstancePos.Z - cameraPos.Z) / rayDirection.Z);
                    rotationStartPos.X = cameraPos.X + t * rayDirection.X;
                    rotationStartPos.Y = cameraPos.Y + t * rayDirection.Y;
                    instancePosToRotationStartPosVec = rotationStartPos - boundInstancePos;
                }
                else
                    instancePosToRotationStartPosVec = cameraPos - boundInstancePos;

                prevCursorPos = new Point(x, y);
            }

            private void setDxHandlesTranslation(Vector3D translation)
            {
                foreach (DxVisualizer.IScene.ISceneModelInstance dxHandle in dxHandles)
                    dxHandle.setTranslation(translation);
            }

            private void updateDxHandlesScale()
            {
                float scale = (float)(Math.Tan(TASL_IN_VISUAL_DEGS_TO_FOV_RATIO * dxScene.getCameraFOV()) * ((Point3D)dxScene.getCameraPos() - (Point3D)boundInstance.Translate.Vector3DCpy).Length);
                foreach (DxVisualizer.IScene.ISceneModelInstance dxHandle in dxHandles)
                    dxHandle.setScale(new Vector3D(scale, scale, scale));
            }
        }        

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
        private SceneModelInstanceDxManipulator manipulator = null;
        private List<SceneModelInstanceDxManipulator> manipulators = new List<SceneModelInstanceDxManipulator>();

        public ObservableCollection<ModelM> ModelMs { get; } = new ObservableCollection<ModelM>();
        public ObservableCollection<SceneM> SceneMs { get; } = new ObservableCollection<SceneM>();

        public double CameraFieldOfView { get; } = SCENE_CAMERA_FOV;
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
                    {
                        selectedScene.IDxScene.activate();
                        manipulator = manipulators[SceneMs.IndexOf(selectedScene)];
                    }
                    else
                        manipulator = null;

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
                    if (selectedSceneModelInstance != null)
                    {
                        selectedSceneModelInstance.dim();
                        manipulator.unbind();
                    }

                    selectedSceneModelInstance = value;
                    if (selectedSceneModelInstance != null)
                    {
                        selectedSceneModelInstance.highlight();
                        manipulator.bindSceneModelInstance(selectedSceneModelInstance);
                    }
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
        public ICommand ToggleSceneModelInstanceVisibilityCmd { get; }
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
        public ICommand CaptureFrameCmd { get; }

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
            AddSceneModelInstanceCmd = new RelayCommand(p => addSceneModelInstance(), p => SelectedSceneModel != null);
            ToggleSceneModelInstanceVisibilityCmd = new RelayCommand(p => toggleSceneModelInstanceVisibility(), p => SelectedSceneModelInstance != null);
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
            CaptureFrameCmd = new RelayCommand(p => captureFrame());
            
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
                    dxVisualizer.addModel(importedModelData.importData.meshesVertices[0], 
                        importedModelData.importData.meshesVertexIndices[0], 
                        Color.FromArgb(255, 175, 175, 175),
                        importedModelData.importData.boundingSphereCenter, 
                        importedModelData.importData.boundingSphereRadius, 
                        PrimitiveTopology.TRIANGLELIST, true, out dxModelID);
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
            manipulators.Add(new SceneModelInstanceDxManipulator(dxVisualizer, addedSceneM.IDxScene));
            SelectedScene = addedSceneM;                        
        }

        private void removeScene()
        {
            manipulators.Remove(manipulator);
            manipulator.releaseDxLmnts();
            manipulator = null;
            SceneMs.Remove(SelectedScene);
            SelectedScene.releaseDxLmnts();                        
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

            dxVisualizer.addModel(vertices, vertexIndices, Color.FromArgb(255, 255, 255, 255), new Point3D(0, 0, 0), GRID_HALF_WIDHT, PrimitiveTopology.LINELIST, true, out gridDxModelId);
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
                CommandManager.InvalidateRequerySuggested();
            }
        }

        private void sceneModelDragOverViewport(DragEventArgs e)
        {            
            if (draggedSceneModelInstance != null)
            {                                
                Vector3D draggedSceneModelTranslation = cursorPosToDraggedModelTranslation(e.GetPosition((FrameworkElement)e.OriginalSource));
                draggedSceneModelInstance.setTranslation(draggedSceneModelTranslation);                
            }
        }

        private Vector3D cursorPosToDraggedModelTranslation(Point cursorPos)
        {
            Vector3D rayDirection = (Vector3D)SelectedScene.IDxScene.cursorPosToRayDirection((float)cursorPos.X, (float)cursorPos.Y);
            return (Vector3D)((Point3D)SelectedScene.IDxScene.getCameraPos() + (SCENE_MODEL_INSTANCE_INIT_DIST_FROM_CAMERA * rayDirection));
        }

        private void sceneModelDropOnViewport(DragEventArgs e)
        {
            draggedSceneModelInstance = null;
            CommandManager.InvalidateRequerySuggested();
        }

        private void addSceneModelInstance()
        {
            SelectedSceneModel.addSceneModelInstance(new Vector3D(0, 0, 0), new Vector3D(1, 1, 1), new Vector3D(1, 0, 0), 0, onSceneModelInstanceSelected);
        }

        private void onSceneModelInstanceSelected(SceneModelInstanceM selectedSceneModelInstance)
        {
            SelectedSceneModelInstance = selectedSceneModelInstance;
        }
        private void toggleSceneModelInstanceVisibility()
        {
            SelectedSceneModelInstance.toggleVisibility();
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
                else if (manipulator.IsActivated)
                    prevMousePos = e.GetPosition(Application.Current.MainWindow);
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

        private void mouseUpSceneViewport(MouseButtonEventArgs e)
        {
            if (cameraAction == CameraAction.ROTATE && e.RightButton == MouseButtonState.Released || cameraAction == CameraAction.PAN && e.MiddleButton == MouseButtonState.Released)
            {
                cameraAction = CameraAction.REST;
                Mouse.Capture(null);
            }
            else if (manipulator.IsActivated && e.LeftButton == MouseButtonState.Released)            
                manipulator.deactivate();            
        }

        private void mouseMoveSceneViewport(MouseEventArgs e)
        {                  
            if (cameraAction != CameraAction.REST || manipulator.IsActivated)
            {
                Point currMousePos = e.GetPosition(Application.Current.MainWindow);
                Vector cursorMoveVec = currMousePos - prevMousePos;                
                if (Math.Abs(cursorMoveVec.X) > DOUBLE_EPSILON || Math.Abs(cursorMoveVec.Y) > DOUBLE_EPSILON)
                {
                    if (cameraAction != CameraAction.REST)
                    {
                        if (cameraAction == CameraAction.ROTATE)
                            selectedScene.IDxScene.rotateCamera((float)cursorMoveVec.X, (float)cursorMoveVec.Y);
                        else if (cameraAction == CameraAction.PAN)
                        {
                            selectedScene.IDxScene.panCamera((float)cursorMoveVec.X, (float)cursorMoveVec.Y);
                            if (manipulator != null)
                                manipulator.onCameraTranslation();
                        }

                    }

                    if (manipulator != null && manipulator.IsActivated)
                        manipulator.onMouseMove(e.GetPosition((FrameworkElement)e.Source));
                }

                prevMousePos = currMousePos;
            }
        }

        private void zoomCamera(MouseWheelEventArgs e)
        {
            if (cameraAction == CameraAction.REST)
            {
                selectedScene.IDxScene.zoomCamera(e.Delta);
                if (manipulator != null)
                    manipulator.onCameraTranslation();
            }
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

        private void captureFrame()
        {
            dxVisualizer.captureFrame();
        }
    }
}
 