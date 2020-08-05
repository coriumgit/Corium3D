using System;
using System.Collections.Generic;
using System.IO;
using Corium3D;
using System.Windows.Media.Media3D;
using System.Windows;
using System.Security.RightsManagement;
using System.Windows.Media;

namespace Corium3DGI
{
	public class AssetsImporter
	{		
		private static AssetsImporter instance;
		public static AssetsImporter Instance
		{
			get
			{
				if (instance == null)
					instance = new AssetsImporter();

				return instance;
			}
		}

		public struct NamedModelData
		{
			public string name;
			public ModelImporter.ImportData importData;
		}

		private struct ModelImport
		{
			public ModelImporter modelImporter;
			public ModelImporter.ImportData importData;
		}

		private Dictionary<String, ModelImport> imports = new Dictionary<string, ModelImport>();

		private AssetsImporter() {}

		public NamedModelData importModel(string colladaPath)
		{
			ModelImport import = new ModelImport();
			import.importData = new ModelImporter.ImportData();
			string modelName = System.IO.Path.GetFileNameWithoutExtension(colladaPath);
			if (imports.ContainsKey(modelName))
				import = imports[modelName];
			else
			{				
				import.modelImporter = new ModelImporter(colladaPath, out import.importData);
				imports.Add(modelName, import);
			}

			return new NamedModelData { name = modelName, importData = import.importData };
		}

		public bool removeModel(string modelName)
		{
			imports.Remove(modelName);

			return true;
		}

		public void clearCollisionPrimitive3D(string modelName)
		{
			ModelImporter modelImporter = imports[modelName].modelImporter;
			if (modelImporter != null)
				modelImporter.clearCollisionPrimitive3D();
		}

		public void assignCollisionBox(string modelName, Point3D center, Point3D scale)
		{
			ModelImporter modelImporter = imports[modelName].modelImporter;
			if (modelImporter != null)
				modelImporter.assignCollisionBox(center, scale);
		}

		public void assignCollisionSphere(string modelName, Point3D center, float radius)
		{
			ModelImporter modelImporter = imports[modelName].modelImporter;
			if (modelImporter != null)
				modelImporter.assignCollisionSphere(center, radius);
		}

		public void assignCollisionCapsule(string modelName, Point3D center1, Vector3D axisVec, float radius)
		{
			ModelImporter modelImporter = imports[modelName].modelImporter;
			if (modelImporter != null)
				modelImporter.assignCollisionCapsule(center1, axisVec, radius);
		}

		public void clearCollisionPrimitive2D(string modelName)
		{
			ModelImporter modelImporter = imports[modelName].modelImporter;
			if (modelImporter != null)
				modelImporter.clearCollisionPrimitive2D();
		}

		public void assignCollisionRect(string modelName, Point center, Point scale)
		{
			ModelImporter modelImporter = imports[modelName].modelImporter;
			if (modelImporter != null)
				modelImporter.assignCollisionRect(center, scale);
		}

		public void assignCollisionCircle(string modelName, Point center, float radius)
		{
			ModelImporter modelImporter = imports[modelName].modelImporter;
			if (modelImporter != null)
				modelImporter.assignCollisionCircle(center, radius);
		}

		public void assignCollisionStadium(string modelName, Point center1, Vector axisVec, float radius)
		{
			ModelImporter modelImporter = imports[modelName].modelImporter;
			if (modelImporter != null)
				modelImporter.assignCollisionStadium(center1, axisVec, radius);
		}

		public void saveImportedModels(string savePath)
		{
			foreach (ModelImport import in imports.Values)
				import.modelImporter.genFiles(savePath);			
		}
	}
}