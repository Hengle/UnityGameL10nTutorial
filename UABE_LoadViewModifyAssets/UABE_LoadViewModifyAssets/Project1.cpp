// Project1.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#include <iostream>
#include <map>
#include <codecvt> // for std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
#include <algorithm>    // std::find
#include <fstream> // for copyFileCustom

#include "AssetsTools\AssetsFileReader.h"
#include "AssetsTools\AssetsFileFormat.h"
#include "AssetsTools/ClassDatabaseFile.h"
#include "AssetsTools/AssetsFileTable.h"
#include "AssetsTools/ResourceManagerFile.h"
#include "AssetsTools/AssetTypeClass.h"
#include "Project1.h"
#include "json/json.h"
//#include "GeneralPurposeFunctions.h"

#include "UnityL10nToolCpp.h"

using namespace std;

ClassDatabaseFile* BasicClassDatabaseFile;
ClassDatabaseFile* MonoClassDatabaseFile;
ResourceManagerFile* ResourceManagerFileGlobal;
AssetsFileTable* GlobalgamemanagersAssetsTable;
vector<string> AssetsFileNames;
map <string, AssetsFile*> FindAssetsFilesFromAssetsName;
map <string, AssetsFileTable*> FindAssetsFileTablesFromAssetsName;
map <INT32, UINT32> FindBasicClassIndexFromClassID;
map <string, UINT32> FindBasicClassIndexFromClassName;
map<UINT64, string> FindMonoClassNameFromMonoScriptPathId;
map<string, UINT32> FindMonoClassIndexFromMonoClassName;

map<string, vector<AssetsReplacer*>> AssetsReplacerForEachAssets;

// Unity Asset 특화 함수

bool ProcessResourceAndMonoManger(
	AssetsFileTable* globalgamemanagersTable,
	wstring gameFolderPath,
	string globalgamemanagersName) {
	AssetsFile* globalgamemanagersFile = globalgamemanagersTable->getAssetsFile();
	int ResourceManagerClassId;
	int MonoManagerClassId;
	vector<string> AssemblyNames;

	ResourceManagerClassId = FindBasicClassIndexFromClassName["ResourceManager"];
	MonoManagerClassId = FindBasicClassIndexFromClassName["MonoManager"];
	int AssetSearchCount = 2;
	for (unsigned int i = 0;
		(i < globalgamemanagersTable->assetFileInfoCount) && AssetSearchCount>0;
		i++) {
		AssetFileInfoEx* assetFileInfoEx = &globalgamemanagersTable->pAssetFileInfo[i];
		int classId;
		WORD monoId;
		GetClassIdFromAssetFileInfoEx(GlobalgamemanagersAssetsTable, assetFileInfoEx, classId, monoId);
		if (classId == ResourceManagerClassId) {
			//assetFileInfoEx.absolutePos
			ResourceManagerFileGlobal = new ResourceManagerFile();
			/*AssetFile* resourceManagerAssetFile = new AssetFile;
			globalgamemanagersFile->GetAssetFile(assetFileInfoEx.absolutePos, globalgamemanagersTable->getReader(), resourceManagerAssetFile);*/
			std::ifstream ifsGlobalgamemanagers(gameFolderPath+WideMultiStringConverter.from_bytes(globalgamemanagersName), std::ios::binary | std::ios::ate);
			ifsGlobalgamemanagers.seekg(assetFileInfoEx->absolutePos, std::ios::beg);

			std::vector<char> resourceManagerBuffer(assetFileInfoEx->curFileSize);
			if (ifsGlobalgamemanagers.read(resourceManagerBuffer.data(), assetFileInfoEx->curFileSize))
			{
				/* worked! */
			}
			int* resourceManagerFilePos = new int(0);
			ResourceManagerFileGlobal->Read(
				(void*)resourceManagerBuffer.data(),
				assetFileInfoEx->curFileSize,
				resourceManagerFilePos,
				globalgamemanagersFile->header.format,
				globalgamemanagersFile->header.endianness ? true : false);
			AssetSearchCount--;
		}
		else if (classId == MonoManagerClassId) {
			AssetTypeTemplateField* baseAssetTypeTemplateField = new AssetTypeTemplateField;
			baseAssetTypeTemplateField->FromClassDatabase(BasicClassDatabaseFile, &BasicClassDatabaseFile->classes[FindBasicClassIndexFromClassID[classId]], (DWORD)0, false);
			AssetTypeInstance baseAssetTypeInstance(
				(DWORD)1, 
				&baseAssetTypeTemplateField, 
				assetFileInfoEx->curFileSize, 
				globalgamemanagersTable->getReader(), 
				globalgamemanagersFile->header.endianness ? true : false, 
				assetFileInfoEx->absolutePos);
			AssetTypeValueField* baseAssetTypeValueField = baseAssetTypeInstance.GetBaseField();
			if (baseAssetTypeValueField) {
				AssetTypeValueField* m_AssemblyNamesArrayATVF =
					baseAssetTypeValueField->Get("m_AssemblyNames")->Get("Array");
				if (m_AssemblyNamesArrayATVF) {
					AssetTypeValueField** m_AssemblyNamesChildrenListATVF = m_AssemblyNamesArrayATVF->GetChildrenList();
					for (DWORD i = 0; i < m_AssemblyNamesArrayATVF->GetChildrenCount(); i++) {
						AssemblyNames.push_back(m_AssemblyNamesChildrenListATVF[i]->GetValue()->AsString());
					}
				}
			}
			AssetSearchCount--;
		}
	}
	LoadMonoClassDatabase(gameFolderPath, AssemblyNames);
	return true;
}

bool LoadMonoClassDatabase(wstring gameFolderPath, vector<string> AssemblyNames) {
	wstring classDatabaseFileName = L"behaviourdb.dat";
	_wremove(classDatabaseFileName.c_str());

	wstring TypeTreeGeneratorParams;
	for (vector<string>::iterator iterator = AssemblyNames.begin(); iterator != AssemblyNames.end(); iterator++) {
		if (!(iterator->empty())) {
			TypeTreeGeneratorParams += L"-f \"" + gameFolderPath + L"Managed\\" + WideMultiStringConverter.from_bytes(*iterator) + L"\" ";
		}
	}
	//TypeTreeGeneratorParams += " 2>&1 > baseList.txt";
	CreateProcessCustom(L".\\Resource\\TypeTreeGenerator.exe " + TypeTreeGeneratorParams);
	// behaviourdb.dat

	IAssetsReader* classDatabaseReader = Create_AssetsReaderFromFile((classDatabaseFileName).c_str(), true, RWOpenFlags_None);
	MonoClassDatabaseFile = new ClassDatabaseFile();
	MonoClassDatabaseFile->Read(classDatabaseReader);
	for (size_t i = 0; i < MonoClassDatabaseFile->classes.size(); i++)
	{
		string monoClassDatabaseTypeName = string(MonoClassDatabaseFile->classes[i].name.GetString(MonoClassDatabaseFile));
		FindMonoClassIndexFromMonoClassName.insert(map<string, unsigned int>::value_type(monoClassDatabaseTypeName, (unsigned int)i));
	}
	return true;
}

bool LoadFindMonoClassNameFromMonoScriptPathId(AssetsFileTable* globalgamemanagersAssetsTable) {
	AssetsFile* globalgamemanagersAssetsFile = globalgamemanagersAssetsTable->getAssetsFile();
	int MonoScriptClassId = FindBasicClassIndexFromClassName["MonoScript"];
	for (unsigned int i = 0; i < globalgamemanagersAssetsTable->assetFileInfoCount; i++) {
		int classId;
		UINT16 monoClassId;
		AssetFileInfoEx* assetFileInfoEx = &globalgamemanagersAssetsTable->pAssetFileInfo[i];
		GetClassIdFromAssetFileInfoEx(globalgamemanagersAssetsTable, assetFileInfoEx, classId, monoClassId);
		if (classId == MonoScriptClassId) {
			AssetTypeTemplateField* baseAssetTypeTemplateField = new AssetTypeTemplateField;
			baseAssetTypeTemplateField->FromClassDatabase(BasicClassDatabaseFile, &BasicClassDatabaseFile->classes[FindBasicClassIndexFromClassID[classId]], (DWORD)0, false);
			AssetTypeInstance baseAssetTypeInstance(
				(DWORD)1,
				&baseAssetTypeTemplateField,
				assetFileInfoEx->curFileSize,
				globalgamemanagersAssetsTable->getReader(),
				globalgamemanagersAssetsFile->header.endianness ? true : false,
				assetFileInfoEx->absolutePos);
			AssetTypeValueField* baseAssetTypeValueField = baseAssetTypeInstance.GetBaseField();
			if (baseAssetTypeValueField) {
				AssetTypeValueField* m_ClassNameATVF = baseAssetTypeValueField->Get("m_ClassName");
				AssetTypeValueField* m_NamespaceATVF = baseAssetTypeValueField->Get("m_Namespace");
				if (m_ClassNameATVF && m_NamespaceATVF) {
					string monoScriptFullName = string(m_NamespaceATVF->GetValue()->AsString()) + "." + m_ClassNameATVF->GetValue()->AsString();
					FindMonoClassNameFromMonoScriptPathId.insert(pair<unsigned __int64, string>(assetFileInfoEx->index, monoScriptFullName));
				}
			}
		}
	}
	return true;
}

bool LoadClassDatabase(string version) {
	size_t lastDotOffset = version.find_last_of('.');
	wstring filter = L"ClassDatabase\\U" + WideMultiStringConverter.from_bytes(version.substr(0, lastDotOffset + 1)) + L"*.dat";
	vector<wstring> classDatabasePathList = get_all_files_names_within_folder(filter);
	wstring classDatabaseFileName = classDatabasePathList[0];
	IAssetsReader* classDatabaseReader = Create_AssetsReaderFromFile((L"ClassDatabase\\" + classDatabaseFileName).c_str(), true, RWOpenFlags_None);
	BasicClassDatabaseFile = new ClassDatabaseFile();
	BasicClassDatabaseFile->Read(classDatabaseReader);
	for (size_t i = 0; i < BasicClassDatabaseFile->classes.size(); i++)
	{
		int classid = BasicClassDatabaseFile->classes[i].classId;
		FindBasicClassIndexFromClassID.insert(map<int, unsigned int>::value_type(classid, (unsigned int)i));
		const char* classDatabaseTypeName = BasicClassDatabaseFile->classes[i].name.GetString(BasicClassDatabaseFile);
		FindBasicClassIndexFromClassName.insert(map<string, UINT32>::value_type(classDatabaseTypeName, (UINT32)i));
	}
	return true;
}

/* gameFolderPath should end by \ */
bool LoadAssetsFile(wstring gameFolderPath, string assetsFileName) {
	if (gameFolderPath.back() != '\\') {
		gameFolderPath += '\\';
	}
	map<string, AssetsFile*>::iterator iterator = FindAssetsFilesFromAssetsName.find(assetsFileName);
	if (iterator == FindAssetsFilesFromAssetsName.end()) {
		IAssetsReader* assetsReader = Create_AssetsReaderFromFile((gameFolderPath + WideMultiStringConverter.from_bytes(assetsFileName)).c_str(), true, RWOpenFlags_None);
		AssetsFile* assetsFile = new AssetsFile(assetsReader);
		AssetsFileTable* assetsFileTable = new AssetsFileTable(assetsFile);
		assetsFileTable->GenerateQuickLookupTree();
		FindAssetsFilesFromAssetsName.insert(pair<string, AssetsFile*>(assetsFileName, assetsFile));
		FindAssetsFileTablesFromAssetsName.insert(pair<string, AssetsFileTable*>(assetsFileName, assetsFileTable));
		AssetsFileNames.push_back(assetsFileName);
		if (assetsFileName == "globalgamemanagers") {
			GlobalgamemanagersAssetsTable = assetsFileTable;
			LoadClassDatabase(assetsFile->typeTree.unityVersion);
			ProcessResourceAndMonoManger(assetsFileTable, gameFolderPath, assetsFileName);
		}
		else if (assetsFileName == "globalgamemanagers.assets") {
			LoadFindMonoClassNameFromMonoScriptPathId(assetsFileTable);
		}
		DWORD dependencyCount = assetsFile->dependencies.dependencyCount;
		if (dependencyCount > 0) {
			for (DWORD i = 0; i < dependencyCount; i++) {
				string newAssetsFileName = assetsFile->dependencies.pDependencies[i].assetPath;
				LoadAssetsFile(gameFolderPath, newAssetsFileName);
			}
			return true;
		}
		else {
			return true;
		}
	}
	else {
		return true;
	}
}

string GetJsonFromAssetTypeValueFieldRecursive(AssetTypeValueField *field) {
	AssetTypeTemplateField* templateField = field->GetTemplateField();
	AssetTypeValueField** fieldChildren = field->GetChildrenList();
	DWORD childrenCount = field->GetChildrenCount();
	string str;
	if (templateField->isArray) {
		if (childrenCount == 0) {
			str = "[";
		}
		else {
			str = "[\r\n";
		}
	}
	else {
		str = "{\r\n";
	}
	
	for (DWORD i = 0; i < childrenCount; i++) {
		AssetTypeValueField* fieldChild = fieldChildren[i];
		AssetTypeTemplateField* templateFieldChild = fieldChild->GetTemplateField();
		string align;
		if (templateFieldChild->align || templateFieldChild->valueType == EnumValueTypes::ValueType_String) {
			align = "1";
		}
		else {
			align = "0";
		}
		string key = align + " " + string(templateFieldChild->type) + " " + string(templateFieldChild->name);
		string value;
		switch (templateFieldChild->valueType) {
		case EnumValueTypes::ValueType_None:
			if (templateFieldChild->isArray) {
				value = GetJsonFromAssetTypeValueFieldRecursive(fieldChild);
			} else {
				value = "\r\n" + GetJsonFromAssetTypeValueFieldRecursive(fieldChild);
			}
			break;
		case EnumValueTypes::ValueType_Int8:
		case EnumValueTypes::ValueType_Int16:
		case EnumValueTypes::ValueType_Int32:
		case EnumValueTypes::ValueType_Int64:
			value = to_string((long long)fieldChild->GetValue()->AsInt());
			break;
		case EnumValueTypes::ValueType_UInt8:
		case EnumValueTypes::ValueType_UInt16:
		case EnumValueTypes::ValueType_UInt32:
		case EnumValueTypes::ValueType_UInt64:
			value = to_string(fieldChild->GetValue()->AsUInt64());
			break;
		case EnumValueTypes::ValueType_Float:
			value = to_string((long double)fieldChild->GetValue()->AsFloat());
			break;
		case EnumValueTypes::ValueType_Double:
			value = to_string((long double)fieldChild->GetValue()->AsDouble());
			break;
		case EnumValueTypes::ValueType_Bool:
			if (fieldChild->GetValue()->AsBool()) {
				value = "true";
			}
			else {
				value = "false";
			}
			break;
		case EnumValueTypes::ValueType_String:
			value = "\"" + string(fieldChild->GetValue()->AsString()) + "\"";
			break;
		}
		if (templateField->isArray) {
			str += "    {\"" + key + "\": ";
			str += ReplaceAll(value, "\r\n", "\r\n    ");
			str += "}";
			if ((i + 1) < childrenCount) {
				str += ",";
				str += "\r\n";
			}
		}
		else {
			str += "    \"" + key + "\": ";
			str += ReplaceAll(value, "\r\n", "\r\n    ");
			if ((i + 1) < childrenCount) {
				str += ",";
				str += "\r\n";
			};
		}
	}
	if (templateField->isArray) {
		if (childrenCount == 0) {
			str += "]";
		}
		else {
			str += "\r\n]";
		}
	}
	else {
		str += "\r\n}";
	}
	return str;
}

string GetJsonFromAssetTypeValueField(AssetTypeValueField *field) {
	string str = "{\r\n";
	AssetTypeTemplateField* templateField = field->GetTemplateField();
	string key = string(templateField->align ? "1" : "0") + " " + string(templateField->type) + " " + string(templateField->name);
	str += "    \"" + key + "\": \r\n    ";
	string value = GetJsonFromAssetTypeValueFieldRecursive(field);
	str += ReplaceAll(value, "\r\n", "\r\n    ");
	str += "\r\n}";
	return str;
}

AssetTypeValueField* GetAssetTypeValueFieldFromJsonRecursive(AssetTypeTemplateField* assetTypeTemplateField, Json::Value json) {
	vector<AssetTypeValueField*>* assetTypeValueFieldArray = new vector<AssetTypeValueField*>();
	AssetTypeValue* assetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_None, 0);
	AssetTypeValueField* assetTypeValueField = new AssetTypeValueField();
	string align;
	if (assetTypeTemplateField->align || assetTypeTemplateField->valueType == EnumValueTypes::ValueType_String) {
		align = "1";
	}
	else {
		align = "0";
	}
	string key = align + " " + string(assetTypeTemplateField->type) + " " + string(assetTypeTemplateField->name);
	//Json::Value thisJson = json[key];
	Json::Value thisJson = json;
	// 이전코드가 잘못되 수정하는도중 임시로 재할당
	vector<string> testStrs1 = thisJson.getMemberNames();
	/*Json::Value thisJson = */
	for (unsigned int i = 0; i < assetTypeTemplateField->childrenCount; i++) {
		AssetTypeTemplateField* childAssetTypeTemplateField = &assetTypeTemplateField->children[i];
		string alignChild;
		if (childAssetTypeTemplateField->align || childAssetTypeTemplateField->valueType == EnumValueTypes::ValueType_String) {
			alignChild = "1";
		}
		else {
			alignChild = "0";
		}
		string keyChild = alignChild + " " + string(childAssetTypeTemplateField->type) + " " + string(childAssetTypeTemplateField->name);
		//void* container;
		AssetTypeValue* childAssetTypeValue;
		AssetTypeValueField* childAssetTypeValueField = new AssetTypeValueField();
		AssetTypeByteArray* assetTypeByteArray;
		string* tempStr;

		//only test
		INT32 testInt = 0;
		switch (childAssetTypeTemplateField->valueType) {
		case EnumValueTypes::ValueType_Int8:
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_Int8, new INT8((INT8)thisJson[keyChild].asInt()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;
		case EnumValueTypes::ValueType_Int16:
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_Int16, new INT16((INT16)thisJson[keyChild].asInt()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;
		case EnumValueTypes::ValueType_Int32:
			testInt = thisJson[keyChild].asInt();
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_Int32, new INT32((INT32)thisJson[keyChild].asInt()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;
		case EnumValueTypes::ValueType_Int64:
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_Int64, new INT64((INT64)thisJson[keyChild].asInt64()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;

		case EnumValueTypes::ValueType_UInt8:
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_UInt8, new UINT8((UINT8)thisJson[keyChild].asUInt()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;
		case EnumValueTypes::ValueType_UInt16:
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_UInt16, new UINT16((UINT16)thisJson[keyChild].asUInt()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;
		case EnumValueTypes::ValueType_UInt32:
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_UInt32, new UINT32((UINT32)thisJson[keyChild].asUInt()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;
		case EnumValueTypes::ValueType_UInt64:
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_UInt64, new UINT64((UINT64)thisJson[keyChild].asUInt64()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;

		case EnumValueTypes::ValueType_Float:
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_Float, new FLOAT((FLOAT)thisJson[keyChild].asFloat()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;

		case EnumValueTypes::ValueType_Double:
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_Double, new DOUBLE((DOUBLE)thisJson[keyChild].asFloat()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;

		case EnumValueTypes::ValueType_Bool:
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_Bool, new BOOL((BOOL)thisJson[keyChild].asBool()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;

		case EnumValueTypes::ValueType_String:
			tempStr = new string(thisJson[keyChild].asString());
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_String, (void*)tempStr->c_str());
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;

		case EnumValueTypes::ValueType_None:
			if (childAssetTypeTemplateField->isArray) {
				childAssetTypeValueField = GetAssetTypeValueFieldArrayFromJson(childAssetTypeTemplateField, thisJson[keyChild]);
			}
			else {
				childAssetTypeValueField = GetAssetTypeValueFieldFromJsonRecursive(childAssetTypeTemplateField, thisJson[keyChild]);
			}
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;
		case EnumValueTypes::ValueType_Array:
			throw new exception("No implement");
			break;
		}
	}
	assetTypeValueField->Read(assetTypeValue, assetTypeTemplateField, assetTypeValueFieldArray->size(), assetTypeValueFieldArray->data());
	return assetTypeValueField;
}

AssetTypeValueField* GetAssetTypeValueFieldFromJson(AssetTypeTemplateField* assetTypeTemplateField, Json::Value json) {
	return GetAssetTypeValueFieldFromJsonRecursive(assetTypeTemplateField, json[json.getMemberNames()[0]]);
}

AssetTypeValueField* GetAssetTypeValueFieldArrayFromJson(AssetTypeTemplateField* assetTypeTemplateField, Json::Value json) {
	Json::StyledWriter writer;
	string testStr = writer.write(json);
	vector<AssetTypeValueField*>* assetTypeValueFieldArray = new vector<AssetTypeValueField*>();
	for (Json::ArrayIndex i = 0; i < json.size(); i++) {
		Json::Value childJson = json[i];
		string key = childJson.getMemberNames()[0];
		assetTypeValueFieldArray->push_back(GetAssetTypeValueFieldFromJsonRecursive(&assetTypeTemplateField->children[1], childJson[key]));
	}
	
	AssetTypeArray* assetTypeArray = new AssetTypeArray();
	assetTypeArray->size = assetTypeValueFieldArray->size();
	AssetTypeValueField* assetTypeValueField = new AssetTypeValueField();
	assetTypeValueField->Read(new AssetTypeValue(EnumValueTypes::ValueType_Array, assetTypeArray), assetTypeTemplateField, assetTypeValueFieldArray->size(), assetTypeValueFieldArray->data());

	return assetTypeValueField;

}


AssetTypeTemplateField* GetMonoAssetTypeTemplateFieldFromClassName(string MonoClassName) {
	int indexOfMonoclass = FindMonoClassIndexFromMonoClassName.find(MonoClassName)->second;

	AssetTypeTemplateField* baseAssetTypeTemplateField = new AssetTypeTemplateField;
	baseAssetTypeTemplateField->FromClassDatabase(BasicClassDatabaseFile, &BasicClassDatabaseFile->classes[FindBasicClassIndexFromClassID[0x72]], (DWORD)0, false);
	AssetTypeTemplateField* baseMonoTypeTemplateField = new AssetTypeTemplateField;
	baseMonoTypeTemplateField->FromClassDatabase(MonoClassDatabaseFile, &MonoClassDatabaseFile->classes[indexOfMonoclass], (DWORD)0, true);
	int prevBaseAssetTypeTemplateFieldChildrenCount = baseAssetTypeTemplateField->childrenCount;
	int prevBaseMonoTypeTemplateFieldChildrenCount = baseMonoTypeTemplateField->childrenCount;
	baseAssetTypeTemplateField->AddChildren(prevBaseMonoTypeTemplateFieldChildrenCount);
	for (int i = 0; i < prevBaseMonoTypeTemplateFieldChildrenCount; i++) {
		baseAssetTypeTemplateField->children[prevBaseAssetTypeTemplateFieldChildrenCount + i] =
			baseMonoTypeTemplateField->children[i];
	}
	return baseAssetTypeTemplateField;
}

string GetClassNameFromBaseAssetTypeValueField(AssetTypeValueField* baseAssetTypeValueField) {
	if (baseAssetTypeValueField) {
		string m_Name = baseAssetTypeValueField->Get("m_Name")->GetValue()->AsString();
		AssetTypeValueField* m_ScriptATVF = baseAssetTypeValueField->Get("m_Script");
		if (m_ScriptATVF) {
			int m_FileId = m_ScriptATVF->Get("m_FileID")->GetValue()->AsInt();
			unsigned __int64 m_PathID = m_ScriptATVF->Get("m_PathID")->GetValue()->AsUInt64();
			return FindMonoClassNameFromMonoScriptPathId.find(m_PathID)->second;
		}
		else {
			throw new exception("GetClassNameFromBaseAssetTypeValueField: m_ScriptATVF not exist");
		}
	}
	else {
		throw new exception("GetClassNameFromBaseAssetTypeValueField: baseAssetTypeValueField not exist");
	}
}

void GetClassIdFromAssetFileInfoEx(AssetsFileTable* assetsFileTable, AssetFileInfoEx* assetFileInfoEx, int& classId, UINT16& monoClassId) {
	if (assetsFileTable->getAssetsFile()->header.format <= 0x10) {
		classId = assetFileInfoEx->curFileType;
	}
	else {
		classId = assetsFileTable->getAssetsFile()->typeTree.pTypes_Unity5[assetFileInfoEx->curFileTypeOrIndex].classId;
		if (classId == 0x72) {
			monoClassId = (WORD)(0xFFFFFFFF - assetFileInfoEx->curFileType); // same as monoScriptIndex in AssetsReplacer
		}
	}
}

AssetTypeInstance* GetBasicAssetTypeInstanceFromAssetFileInfoEx(AssetsFileTable* assetsFileTable, AssetFileInfoEx* assetFileInfoEx) {
	int classId;
	WORD monoClassId;
	GetClassIdFromAssetFileInfoEx(assetsFileTable, assetFileInfoEx, classId, monoClassId);
	AssetTypeTemplateField* baseAssetTypeTemplateField = new AssetTypeTemplateField;
	baseAssetTypeTemplateField->FromClassDatabase(BasicClassDatabaseFile, &BasicClassDatabaseFile->classes[FindBasicClassIndexFromClassID[classId]], (DWORD)0, false);
	AssetTypeInstance* baseAssetTypeInstance = new AssetTypeInstance(
		(DWORD)1,
		&baseAssetTypeTemplateField,
		assetFileInfoEx->curFileSize,
		assetsFileTable->getReader(),
		assetsFileTable->getAssetsFile()->header.endianness ? true : false,
		assetFileInfoEx->absolutePos);
	return baseAssetTypeInstance;
}

AssetTypeInstance* GetDetailAssetTypeInstanceFromAssetFileInfoEx(AssetsFileTable* assetsFileTable, AssetFileInfoEx* assetFileInfoEx) {
	int classId;
	WORD monoClassId;
	GetClassIdFromAssetFileInfoEx(assetsFileTable, assetFileInfoEx, classId, monoClassId);
	AssetTypeInstance* baseAssetTypeInstance = GetBasicAssetTypeInstanceFromAssetFileInfoEx(assetsFileTable, assetFileInfoEx);
	if (classId == 0x72) {
		AssetTypeValueField* baseAssetTypeValueField = baseAssetTypeInstance->GetBaseField();
		string monoScriptFullName = GetClassNameFromBaseAssetTypeValueField(baseAssetTypeValueField);
		AssetTypeTemplateField* baseMonoTypeTemplateField = GetMonoAssetTypeTemplateFieldFromClassName(monoScriptFullName);
		AssetTypeInstance* baseMonoTypeInstance = new AssetTypeInstance(
			(DWORD)1,
			&baseMonoTypeTemplateField,
			assetFileInfoEx->curFileSize,
			assetsFileTable->getReader(),
			assetsFileTable->getAssetsFile()->header.endianness ? true : false,
			assetFileInfoEx->absolutePos);
		return baseMonoTypeInstance;
	}
	else {
		return baseAssetTypeInstance;
	}
}

string GetJsonKeyFromAssetTypeTemplateField(AssetTypeTemplateField* assetTypeTemplateField) {
	string align;
	if (assetTypeTemplateField->align || assetTypeTemplateField->valueType == EnumValueTypes::ValueType_String) {
		align = "1";
	}
	else {
		align = "0";
	}
	string key = align + " " + string(assetTypeTemplateField->type) + " " + string(assetTypeTemplateField->name);
	return key;
}

string GetJsonKeyFromAssetTypeValueField(AssetTypeValueField* assetTypeValueField) {
	return GetJsonKeyFromAssetTypeTemplateField(assetTypeValueField->GetTemplateField());
}

bool ModifyAssetTypeValueFieldFromJSONRecursive(AssetTypeValueField* assetTypeValueField, Json::Value json) {
	string key = GetJsonKeyFromAssetTypeValueField(assetTypeValueField);
	vector<string> jsonKeyList = json.getMemberNames();
	for (unsigned int i = 0; i < assetTypeValueField->GetChildrenCount(); i++) {
		AssetTypeValueField* childAssetTypeValueField = assetTypeValueField->GetChildrenList()[i];
		string keyChild = GetJsonKeyFromAssetTypeValueField(childAssetTypeValueField);
		vector<string>::iterator iterator = find(jsonKeyList.begin(), jsonKeyList.end(), keyChild);

		if (iterator != jsonKeyList.end()) {
			switch (childAssetTypeValueField->GetTemplateField()->valueType) {
				case EnumValueTypes::ValueType_Int8:
					childAssetTypeValueField->GetValue()->Set(new INT8((INT8)json[keyChild].asInt()));
					break;
				case EnumValueTypes::ValueType_Int16:
					childAssetTypeValueField->GetValue()->Set(new INT16((INT16)json[keyChild].asInt()));
					break;
				case EnumValueTypes::ValueType_Int32:
					childAssetTypeValueField->GetValue()->Set(new INT32((INT32)json[keyChild].asInt()));
					break;
				case EnumValueTypes::ValueType_Int64:
					childAssetTypeValueField->GetValue()->Set(new INT64((INT64)json[keyChild].asInt64()));
					break;

				case EnumValueTypes::ValueType_UInt8:
					childAssetTypeValueField->GetValue()->Set(new UINT8((UINT8)json[keyChild].asUInt()));
					break;
				case EnumValueTypes::ValueType_UInt16:
					childAssetTypeValueField->GetValue()->Set(new UINT16((UINT16)json[keyChild].asUInt()));
					break;
				case EnumValueTypes::ValueType_UInt32:
					childAssetTypeValueField->GetValue()->Set(new UINT32((UINT32)json[keyChild].asUInt()));
					break;
				case EnumValueTypes::ValueType_UInt64:
					childAssetTypeValueField->GetValue()->Set(new UINT64((UINT64)json[keyChild].asUInt64()));
					break;

				case EnumValueTypes::ValueType_Float:
					childAssetTypeValueField->GetValue()->Set(new FLOAT((FLOAT)json[keyChild].asFloat()));
					break;
				case EnumValueTypes::ValueType_Double:
					childAssetTypeValueField->GetValue()->Set(new DOUBLE((DOUBLE)json[keyChild].asDouble()));
					break;

				case EnumValueTypes::ValueType_Bool:
					childAssetTypeValueField->GetValue()->Set(new BOOL((BOOL)json[keyChild].asBool()));
					break;

				case EnumValueTypes::ValueType_String:
					childAssetTypeValueField->GetValue()->Set(new string(json[keyChild].asString()));
					break;

				case EnumValueTypes::ValueType_None:
					if (childAssetTypeValueField->GetTemplateField()->isArray) {
						//ClearAssetTypeValueField(childAssetTypeValueField); // 해야할지 안해도 될지 모르겠
						assetTypeValueField->GetChildrenList()[i] = GetAssetTypeValueFieldArrayFromJson(childAssetTypeValueField->GetTemplateField(), json[keyChild]);
					}
					else {
						ModifyAssetTypeValueFieldFromJSONRecursive(childAssetTypeValueField, json[keyChild]);
					}
					break;

				case EnumValueTypes::ValueType_Array:
					throw new exception("No implement");
					break;
			}
		}
	}
	return true;
}

bool ModifyAssetTypeValueFieldFromJSON(AssetTypeValueField* assetTypeValueField, Json::Value json) {
	vector<string> jsonMembers = json.getMemberNames();
	if (jsonMembers.size() == 0) {
		return true;
	}
	else {
		return ModifyAssetTypeValueFieldFromJSONRecursive(assetTypeValueField, json[json.getMemberNames()[0]]);
	}
}

int GetDependencyIndexFromPathId(int PathId) {
	// 이것도 할때마다 찾는건 비효율적임.
	if (PathId - 1 >= (INT32)GlobalgamemanagersAssetsTable->getAssetsFile()->dependencies.dependencyCount) {
		throw exception("globalgamemanagersAssetsTable->getAssetsFile()->dependencies.pDependencies length over");
	}
	string dependencyAssetsPath = string(GlobalgamemanagersAssetsTable->getAssetsFile()->dependencies.pDependencies[PathId-1].assetPath);
	return distance(AssetsFileNames.begin(), find(AssetsFileNames.begin(), AssetsFileNames.end(), dependencyAssetsPath));
}

vector<AssetLogicalPath> GetFontAssetListAsNameAndContainerInternal(AssetsFileTable* assetsFileTable, string AssetsName, int AssetsPathId) {
	vector<AssetLogicalPath> assetLogicalPaths;
	string MonoClassNameToFind = "TMPro.TMP_FontAsset";
	AssetsFile* assetsFile = assetsFileTable->getAssetsFile();
	ResourceManager_ContainerData* resourceManagerFileContainerArray = ResourceManagerFileGlobal->containerArray;

	for (unsigned int i = 0; i < assetsFileTable->assetFileInfoCount; i++)
	{
		AssetFileInfoEx* assetFileInfoEx = &assetsFileTable->pAssetFileInfo[i];
		int classId;
		WORD monoClassId;
		GetClassIdFromAssetFileInfoEx(assetsFileTable, assetFileInfoEx, classId, monoClassId);
		if (classId == 0x72) {
			AssetTypeInstance* baseAssetTypeInstance = GetBasicAssetTypeInstanceFromAssetFileInfoEx(assetsFileTable, assetFileInfoEx);
			AssetTypeValueField* baseAssetTypeValueField = baseAssetTypeInstance->GetBaseField();
			if (baseAssetTypeValueField) {
				string monoClassName = GetClassNameFromBaseAssetTypeValueField(baseAssetTypeValueField);
				if (monoClassName == MonoClassNameToFind) {
					string m_Name = baseAssetTypeValueField->Get("m_Name")->GetValue()->AsString();
					int j = 0;
					for (; j < ResourceManagerFileGlobal->containerArrayLen; j++) {
						if (AssetsPathId == GetDependencyIndexFromPathId(resourceManagerFileContainerArray[j].ids.fileId) &&
							assetFileInfoEx->index == resourceManagerFileContainerArray[j].ids.pathId) {
							AssetLogicalPath assetLogicalPath = { AssetsName, m_Name, resourceManagerFileContainerArray[j].name, true };
							assetLogicalPaths.push_back(assetLogicalPath);
							break;
						}
					}
					if (j == ResourceManagerFileGlobal->containerArrayLen) {
						AssetLogicalPath assetLogicalPath = { AssetsName, m_Name, "", false };
						assetLogicalPaths.push_back(assetLogicalPath);
					}
				}
			}
		}
	}
	return assetLogicalPaths;
}

vector<AssetLogicalPath> GetFontAssetListAsNameAndContainer() {
	vector<AssetLogicalPath> FontAssetList;
	ptrdiff_t ptrdiffResource = distance(AssetsFileNames.begin(), find(AssetsFileNames.begin(), AssetsFileNames.end(), string("resources.assets")));
	AssetsFileTable* resourcesAssetsFileTable = FindAssetsFileTablesFromAssetsName["resources.assets"];
	vector<AssetLogicalPath> resourcesFontAssetList = GetFontAssetListAsNameAndContainerInternal(resourcesAssetsFileTable, "resources.assets", ptrdiffResource);

	ptrdiff_t ptrdiffShared0 = distance(AssetsFileNames.begin(), find(AssetsFileNames.begin(), AssetsFileNames.end(), "sharedassets0.assets"));
	AssetsFileTable* shared0AssetsFileTable = FindAssetsFileTablesFromAssetsName["sharedassets0.assets"];
	vector<AssetLogicalPath> shared0FontAssetList = GetFontAssetListAsNameAndContainerInternal(shared0AssetsFileTable, "sharedassets0.assets", ptrdiffShared0);

	FontAssetList.insert(FontAssetList.end(), resourcesFontAssetList.begin(), resourcesFontAssetList.end());
	FontAssetList.insert(FontAssetList.end(), shared0FontAssetList.begin(), shared0FontAssetList.end());
	return FontAssetList;
}

pair<int, INT64> FindFilePathIdFromContainer(string ContainerPath) {
	ResourceManager_PPtr resourceManager_PPtr;
	for (int i = 0; i < ResourceManagerFileGlobal->containerArrayLen; i++) {
		if (ResourceManagerFileGlobal->containerArray[i].name == ContainerPath) {
			resourceManager_PPtr = ResourceManagerFileGlobal->containerArray[i].ids;
			return pair<int, INT64>(resourceManager_PPtr.fileId, resourceManager_PPtr.pathId);
		}
	}
}

unsigned int FindAssetIndexFromName(AssetsFileTable* assetsFileTable, string assetName) {
	for (unsigned int i = 0; i< assetsFileTable->assetFileInfoCount; i++) {
		AssetFileInfoEx *assetFileInfoEx = &assetsFileTable->pAssetFileInfo[i];
		AssetTypeInstance* assetTypeInstance = GetBasicAssetTypeInstanceFromAssetFileInfoEx(assetsFileTable, assetFileInfoEx);
		AssetTypeValueField* assetTypeValueField = assetTypeInstance->GetBaseField();
		AssetTypeValueField* m_NameATVF = assetTypeValueField->Get("m_Name");
		if (m_NameATVF && !m_NameATVF->IsDummy()) {
			string m_Name = m_NameATVF->GetValue()->AsString();
			if (m_Name == assetName) {
				return i;
			}
		}
		else {
			assetTypeInstance->~AssetTypeInstance();
		}
	}
	return -1;
}

bool MakeModifiedAssetsFile(wstring gameFolderPath) {
	for (map<string, vector<AssetsReplacer*>>::iterator iterator = AssetsReplacerForEachAssets.begin();
		iterator != AssetsReplacerForEachAssets.end(); iterator++) {
		string key = iterator->first;
		vector<AssetsReplacer*> assetsReplacers = iterator->second;
		AssetsFileTable* assetsFileTable = FindAssetsFileTablesFromAssetsName[key];
		if (assetsReplacers.size() > 0) {
			wstring fullPath = gameFolderPath + WideMultiStringConverter.from_bytes(key);
			IAssetsWriter* assetsWriter = Create_AssetsWriterToFile((fullPath + L".mod").c_str(), true, true, RWOpenFlags_None);
			assetsFileTable->getAssetsFile()->Write(assetsWriter, 0, assetsReplacers.data(), assetsReplacers.size(), 0);
			assetsFileTable->getReader()->Close();
			assetsWriter->Close();
			int removeResult = _wremove(fullPath.c_str());
			int renameResult = _wrename((fullPath + L".mod").c_str(), fullPath.c_str());
		}
	}
	return true;
}

bool makeAssetsReplacer(string assetsFileName, AssetsFileTable* assetsFileTable, AssetFileInfoEx* assetFileInfoEx, AssetTypeInstance* assetTypeInstance, wstring replaceAssetPath) {
	INT64 PathId = assetFileInfoEx->index;
	int classId;
	WORD monoClassId;
	AssetTypeValueField* baseAssetTypeValueField = assetTypeInstance->GetBaseField();
	GetClassIdFromAssetFileInfoEx(assetsFileTable, assetFileInfoEx, classId, monoClassId);

	Json::Value modifiedJson;
	JsonReader.parse(readFile2(replaceAssetPath), modifiedJson);
	ModifyAssetTypeValueFieldFromJSON(baseAssetTypeValueField, modifiedJson);

	QWORD newByteSize = baseAssetTypeValueField->GetByteSize(0);
	void* newAssetBuffer = malloc((size_t)newByteSize);
	if (newAssetBuffer) {
		IAssetsWriter *pWriter = Create_AssetsWriterToMemory(newAssetBuffer, (size_t)newByteSize);
		if (pWriter) {
			newByteSize = baseAssetTypeValueField->Write(pWriter, 0, assetsFileTable->getAssetsFile()->header.endianness ? true : false);
			AssetsReplacer *pReplacer = MakeAssetModifierFromMemory(0, PathId, classId, monoClassId, newAssetBuffer, (size_t)newByteSize, free);
			if (pReplacer) {
				AssetsReplacerForEachAssets[assetsFileName].push_back(pReplacer);
				return true;
			}
		}
	}
	return false;
}

bool makeAssetsReplacer(string assetsFileName, AssetsFileTable* assetsFileTable, AssetFileInfoEx* assetFileInfoEx, AssetTypeInstance* assetTypeInstance) {
	INT64 PathId = assetFileInfoEx->index;
	int classId;
	WORD monoClassId;
	AssetTypeValueField* baseAssetTypeValueField = assetTypeInstance->GetBaseField();
	GetClassIdFromAssetFileInfoEx(assetsFileTable, assetFileInfoEx, classId, monoClassId);
	QWORD newByteSize = baseAssetTypeValueField->GetByteSize(0);
	void* newAssetBuffer = malloc((size_t)newByteSize);
	if (newAssetBuffer) {
		IAssetsWriter *pWriter = Create_AssetsWriterToMemory(newAssetBuffer, (size_t)newByteSize);
		if (pWriter) {
			newByteSize = baseAssetTypeValueField->Write(pWriter, 0, assetsFileTable->getAssetsFile()->header.endianness ? true : false);
			AssetsReplacer *pReplacer = MakeAssetModifierFromMemory(0, PathId, classId, monoClassId, newAssetBuffer, (size_t)newByteSize, free);
			if (pReplacer) {
				AssetsReplacerForEachAssets[assetsFileName].push_back(pReplacer);
				return true;
			}
		}
	}
	return false;
}

void ReadReplacerJsonAndMakeReplacerForTMP_FontAsset(wstring gameFolderPath) {
	string jsonStr = readFile2(L"Resource\\TMPro_TMP_Replacer.json");
	Json::Value json;
	JsonReader.parse(jsonStr, json);

	for (Json::ArrayIndex i = 0; i < json.size(); i++) {
		Json::Value assetLogicalReplacerJson = json[i];
		string assetsName = "";
		string assetName = "";
		string containerPath = "";
		bool hasContainerPath = false;
		
		string replaceAssetPath = "";
		bool UseContainerPath = false;
		// for material
		float _TextureHeight;
		float _TextureWidth;
		// for atlas
		int m_Width = 0;
		int m_Height = 0;
		int m_CompleteImageSize;
		string atlasPath = "";


		if (assetLogicalReplacerJson["AssetLogicalPath"].isObject()) {
			if (assetLogicalReplacerJson["AssetLogicalPath"]["AssetsName"].isString()) {
				assetsName = assetLogicalReplacerJson["AssetLogicalPath"]["AssetsName"].asString();
			}
			if (assetLogicalReplacerJson["AssetLogicalPath"]["AssetName"].isString()) {
				assetName = assetLogicalReplacerJson["AssetLogicalPath"]["AssetName"].asString();
			}
			if (assetLogicalReplacerJson["AssetLogicalPath"]["ContainerPath"].isString()) {
				containerPath = assetLogicalReplacerJson["AssetLogicalPath"]["ContainerPath"].asString();
			}
			if (assetLogicalReplacerJson["AssetLogicalPath"]["hasContainerPath"].isBool()) {
				hasContainerPath = assetLogicalReplacerJson["AssetLogicalPath"]["hasContainerPath"].asBool();
			}
		}
		if (assetLogicalReplacerJson["replaceAssetPath"].isString()) {
			replaceAssetPath = assetLogicalReplacerJson["replaceAssetPath"].asString();
		}
		if (assetLogicalReplacerJson["UseContainerPath"].isBool()) {
			UseContainerPath = assetLogicalReplacerJson["UseContainerPath"].asBool();
		}
		if (assetLogicalReplacerJson["_TextureHeight"].isDouble()) {
			_TextureHeight = assetLogicalReplacerJson["_TextureHeight"].asFloat();
		}
		if (assetLogicalReplacerJson["_TextureWidth"].isDouble()) {
			_TextureWidth = assetLogicalReplacerJson["_TextureWidth"].asFloat();
		}
		if (assetLogicalReplacerJson["m_CompleteImageSize"].isInt()) {
			m_CompleteImageSize = assetLogicalReplacerJson["m_CompleteImageSize"].asInt();
		}
		if (assetLogicalReplacerJson["atlasPath"].isString()) {
			atlasPath = assetLogicalReplacerJson["atlasPath"].asString();
		}
		if (assetLogicalReplacerJson["m_Width"].isInt()) {
			m_Width = assetLogicalReplacerJson["m_Width"].asInt();
		}
		if (assetLogicalReplacerJson["m_Height"].isInt()) {
			m_Height = assetLogicalReplacerJson["m_Height"].asInt();
		}


		string assetsFileName;
		AssetsFileTable* assetsFileTable;
		AssetFileInfoEx* assetFileInfoEx;
		AssetTypeInstance* assetTypeInstance;
		AssetTypeValueField* baseAssetTypeValueField;

		INT64 PathId;
		try {
			if (UseContainerPath) {
				pair<int, INT64> FilePathId = FindFilePathIdFromContainer(containerPath);
				int FileIndex = GetDependencyIndexFromPathId(FilePathId.first);
				PathId = FilePathId.second;
				assetsFileName = AssetsFileNames[FileIndex];
				assetsFileTable = FindAssetsFileTablesFromAssetsName[assetsFileName];
				assetFileInfoEx = assetsFileTable->getAssetInfo(PathId);
			}
			else {
				assetsFileName = assetsName;
				assetsFileTable = FindAssetsFileTablesFromAssetsName[assetsFileName];
				unsigned int assetIndex = FindAssetIndexFromName(assetsFileTable, assetName);
				if (assetIndex == -1) {
					continue;
				}
				assetFileInfoEx = &assetsFileTable->pAssetFileInfo[assetIndex];
			}
		}
		catch (exception e) {
			continue;
		}
		if (assetFileInfoEx == NULL) {
			continue;
		}
		assetTypeInstance = GetDetailAssetTypeInstanceFromAssetFileInfoEx(assetsFileTable, assetFileInfoEx);
		baseAssetTypeValueField = assetTypeInstance->GetBaseField();
		if (baseAssetTypeValueField->IsDummy())
		{
			continue;
		}
		makeAssetsReplacer(assetsFileName, assetsFileTable, assetFileInfoEx, assetTypeInstance, WideMultiStringConverter.from_bytes(replaceAssetPath));

		INT64 materialPathId = baseAssetTypeValueField->Get("material")->Get("m_PathID")->GetValue()->AsInt64();
		AssetFileInfoEx* materialAssetFileInfoEx = assetsFileTable->getAssetInfo(materialPathId);
		AssetTypeInstance* materialAssetTypeInstance = GetDetailAssetTypeInstanceFromAssetFileInfoEx(assetsFileTable, materialAssetFileInfoEx);
		ReplaceMaterial(assetsFileName, assetsFileTable, materialAssetFileInfoEx, materialAssetTypeInstance, _TextureHeight, _TextureWidth);

		INT64 atlasPathId = baseAssetTypeValueField->Get("atlas")->Get("m_PathID")->GetValue()->AsInt64();
		AssetFileInfoEx* atlasAssetFileInfoEx = assetsFileTable->getAssetInfo(atlasPathId);
		AssetTypeInstance* atlasAssetTypeInstance = GetDetailAssetTypeInstanceFromAssetFileInfoEx(assetsFileTable, atlasAssetFileInfoEx);
		ReplaceAtlas(assetsFileName, assetsFileTable, atlasAssetFileInfoEx, atlasAssetTypeInstance, m_CompleteImageSize, atlasPath, m_Width, m_Height);
		/*Json::Value relatedAssetJson = assetLogicalReplacerJson["relatedAsset"];
		if (relatedAssetJson.isObject()) {
			vector<string> relatedAssetMemberNames = relatedAssetJson.getMemberNames();
			for (vector<string>::iterator relatedAssetMemberiterator = relatedAssetMemberNames.begin();
				relatedAssetMemberiterator != relatedAssetMemberNames.end(); relatedAssetMemberiterator++) {
				string key = *relatedAssetMemberiterator;
				if (relatedAssetJson[key]["replaceAssetPath"].isString()) {
					string replaceAssetPath2 = relatedAssetJson[key]["replaceAssetPath"].asString();
					WorkForRelatedAsset(assetsName, assetsFileTable, baseAssetTypeValueField, key, replaceAssetPath2);
				}

			}
		}*/

		Json::Value ToCopyJson = assetLogicalReplacerJson["ToCopy"];
		if (ToCopyJson.isObject()) {
			vector<string> ToCopyMemberNames = ToCopyJson.getMemberNames();
			for (vector<string>::iterator ToCopyMemberName = ToCopyMemberNames.begin();
				ToCopyMemberName != ToCopyMemberNames.end(); ToCopyMemberName++) {
				string originalFileName = *ToCopyMemberName;
				wstring DestinationFileName = gameFolderPath + WideMultiStringConverter.from_bytes(ToCopyJson[originalFileName].asString());
				copyFileCustom(WideMultiStringConverter.from_bytes(originalFileName).c_str(), DestinationFileName.c_str());
			}
		}
	}
}

void ReplaceMaterial(string assetsName, AssetsFileTable* assetsFileTable, AssetFileInfoEx* assetFileInfoEx, AssetTypeInstance* assetTypeInstance, float _TextureHeight, float _TextureWidth) {
	AssetTypeValueField* m_FloatsArrayATVF = assetTypeInstance->GetBaseField()->Get("m_SavedProperties")->Get("m_Floats")->Get("Array");
	AssetTypeValueField** m_FloatsATVFChildrenArray = m_FloatsArrayATVF->GetChildrenList();
	int modifyCounter = 2;
	for (unsigned int i = 0; i < m_FloatsArrayATVF->GetChildrenCount() && modifyCounter>0; i++) {
		string first = m_FloatsATVFChildrenArray[i]->Get("first")->GetValue()->AsString();
		if (first == "_TextureHeight") {
			m_FloatsATVFChildrenArray[i]->Get("second")->GetValue()->Set(new float(_TextureHeight));
			modifyCounter--;
		}
		else if (first == "_TextureWidth") {
			m_FloatsATVFChildrenArray[i]->Get("second")->GetValue()->Set(new float(_TextureWidth));
			modifyCounter--;
		}
	}
	makeAssetsReplacer(assetsName, assetsFileTable, assetFileInfoEx, assetTypeInstance);
}

void ReplaceAtlas(string assetsname, AssetsFileTable* assetsFileTable, AssetFileInfoEx* assetFileInfoEx, AssetTypeInstance* assetTypeInstance, int m_CompleteImageSize, string atlasPath, int m_Width, int m_Height) {
	AssetTypeValueField* assetTypeValueField = assetTypeInstance->GetBaseField();
	assetTypeInstance->GetBaseField()->Get("m_Width")->GetValue()->Set(new INT32(m_Width));
	assetTypeInstance->GetBaseField()->Get("m_Height")->GetValue()->Set(new INT32(m_Height));
	assetTypeInstance->GetBaseField()->Get("m_CompleteImageSize")->GetValue()->Set(new INT32(m_CompleteImageSize));
	assetTypeInstance->GetBaseField()->Get("m_StreamData")->Get("offset")->GetValue()->Set(new UINT32(0));
	assetTypeInstance->GetBaseField()->Get("m_StreamData")->Get("size")->GetValue()->Set(new UINT32(m_CompleteImageSize));
	assetTypeInstance->GetBaseField()->Get("m_StreamData")->Get("path")->GetValue()->Set((void*)(new string(atlasPath))->c_str());
	makeAssetsReplacer(assetsname, assetsFileTable, assetFileInfoEx, assetTypeInstance);
}

void test3(wstring gameFolderPath) {
	vector<AssetLogicalPath> TMPFont_AssetLogicalPaths = GetFontAssetListAsNameAndContainer();
	ReadReplacerJsonAndMakeReplacerForTMP_FontAsset(gameFolderPath);
	MakeModifiedAssetsFile(gameFolderPath);
}

int main()
{

	//wchar_t WcharCurrentDirectory[255] = {};
	//_wgetcwd(WcharCurrentDirectory, 255);
	//wstring _currentDirectory(WcharCurrentDirectory);
	//_currentDirectory += L"\\";
	//vector<wstring> testFilter = GetAllFilesFilterWithinAllSubFolder(_currentDirectory + L"Resource\\Plugins\\FontPlugins\\", L"*.testext");

	//string firstAssetsFileName = "globalgamemanagers";
	////wstring _gameFolderPath = L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Slime Rancher\\SlimeRancher_Data\\";
	//wstring _gameFolderPath = L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Slime Rancher - 복사본\\SlimeRancher_Data\\";
	//LoadAssetsFile(_gameFolderPath, firstAssetsFileName);
	//test3(_gameFolderPath);

	wstring _gameFolderPath = L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Slime Rancher - 복사본\\SlimeRancher_Data\\";
	UnityL10nToolCpp unityL10nToolCpp(_gameFolderPath);
	map<wstring, vector<FontAssetMap>> tempPluginsSupportAssetMap = unityL10nToolCpp.GetPluginsSupportAssetMap();

	return 0;
}



// 프로그램 실행: <Ctrl+F5> 또는 [디버그] > [디버깅하지 않고 시작] 메뉴
// 프로그램 디버그: <F5> 키 또는 [디버그] > [디버깅 시작] 메뉴

// 시작을 위한 팁: 
//   1. [솔루션 탐색기] 창을 사용하여 파일을 추가/관리합니다.
//   2. [팀 탐색기] 창을 사용하여 소스 제어에 연결합니다.
//   3. [출력] 창을 사용하여 빌드 출력 및 기타 메시지를 확인합니다.
//   4. [오류 목록] 창을 사용하여 오류를 봅니다.
//   5. [프로젝트] > [새 항목 추가]로 이동하여 새 코드 파일을 만들거나, [프로젝트] > [기존 항목 추가]로 이동하여 기존 코드 파일을 프로젝트에 추가합니다.
//   6. 나중에 이 프로젝트를 다시 열려면 [파일] > [열기] > [프로젝트]로 이동하고 .sln 파일을 선택합니다.
