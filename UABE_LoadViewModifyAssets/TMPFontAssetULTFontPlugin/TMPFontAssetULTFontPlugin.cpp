// TMPFontAssetULTFontPlugin.cpp : DLL 응용 프로그램을 위해 내보낸 함수를 정의합니다.
//

#include "stdafx.h"

#include <string>
#include <algorithm>
using namespace std;

#include "AssetsTools/AssetsFileReader.h"
#include "AssetsTools/AssetsFileFormat.h"
#include "AssetsTools/ClassDatabaseFile.h"
#include "AssetsTools/AssetsFileTable.h"
#include "AssetsTools/ResourceManagerFile.h"
#include "AssetsTools/AssetTypeClass.h"
#include "IULTFontPluginInterface.h"
#include "GeneralPurposeFunctions.h"
#include "json/json.h"

UnityL10nToolAPI* UnityL10nToolAPIGlobal;
FontPluginInfo* FontPluginInfoGlobal;

vector<FontAssetMap> searchedFontAssetMap;
vector<FontAssetMap> selectedFontAssetMapList;
vector<wstring> OptionsList;
Json::Value OptionsJson;
Json::Value ProjectConfig;

bool _cdecl SetProjectConfigJson(Json::Value pluginConfig) {
	ProjectConfig = pluginConfig;
	if (ProjectConfig.isMember("SelectedFontAssetMapList")) {
		Json::Value selectedFontAssetMapListJson = ProjectConfig["SelectedFontAssetMapList"];
		if (selectedFontAssetMapListJson.isArray()) {
			for (Json::ArrayIndex i = 0; i < selectedFontAssetMapListJson.size(); i++) {
				Json::Value selectedFontAssetMapJson = selectedFontAssetMapListJson[i];
				std::string assetsName;
				std::string assetName;
				std::string containerPath;
				std::vector<std::wstring> options;
				std::wstring selectedOption;
				bool useContainerPath = false;

				if (selectedFontAssetMapJson["AssetsName"].isString()) {
					assetsName = selectedFontAssetMapJson["AssetsName"].asString();
				}
				if (selectedFontAssetMapJson["AssetName"].isString()) {
					assetName = selectedFontAssetMapJson["AssetName"].asString();
				}
				if (selectedFontAssetMapJson["ContainerPath"].isString()) {
					containerPath = selectedFontAssetMapJson["ContainerPath"].asString();
				}
				if (selectedFontAssetMapJson["SelectedOption"].isString()) {
					selectedOption = WideMultiStringConverter.from_bytes(selectedFontAssetMapJson["SelectedOption"].asString());
				}
				if (selectedFontAssetMapJson["UseContainerPath"].isBool()) {
					useContainerPath = selectedFontAssetMapJson["UseContainerPath"].isBool();
				}
				FontAssetMap tempFontAssetMap = {
					assetsName,
					assetName,
					containerPath,
					std::vector<std::wstring>(),
					selectedOption,
					useContainerPath
				};
				selectedFontAssetMapList.push_back(tempFontAssetMap);
			}
		}
	}
	return true;
}

vector<FontAssetMap> _cdecl GetPluginSupportAssetMap() {
	vector<FontAssetMap> FontAssetMapListFromResourcesAssets = UnityL10nToolAPIGlobal->GetFontAssetMapListFromMonoClassName("resources.assets", "TMPro.TMP_FontAsset");
	vector<FontAssetMap> FontAssetMapListFromShared0Assets = UnityL10nToolAPIGlobal->GetFontAssetMapListFromMonoClassName("sharedassets0.assets", "TMPro.TMP_FontAsset");
	searchedFontAssetMap.insert(searchedFontAssetMap.end(), FontAssetMapListFromResourcesAssets.begin(), FontAssetMapListFromResourcesAssets.end());
	searchedFontAssetMap.insert(searchedFontAssetMap.end(), FontAssetMapListFromShared0Assets.begin(), FontAssetMapListFromShared0Assets.end());
	for (vector<FontAssetMap>::iterator iterator = searchedFontAssetMap.begin();
		iterator != searchedFontAssetMap.end(); iterator++) {
		iterator->options = OptionsList;
	}
	return searchedFontAssetMap;
}

bool _cdecl SetPluginAssetMap(std::vector<FontAssetMap> fontAssetMapList) {
	selectedFontAssetMapList = fontAssetMapList;
	return true;
}

Json::Value _cdecl GetProjectConfigJson() {
	Json::Value exportJson;
	for (vector<FontAssetMap>::iterator iterator = selectedFontAssetMapList.begin();
		iterator != selectedFontAssetMapList.end(); iterator++) {
		Json::Value tempSelectedFontAssetMapJson;
		tempSelectedFontAssetMapJson["AssetsName"] = iterator->assetsName;
		tempSelectedFontAssetMapJson["AssetName"] = iterator->assetName;
		tempSelectedFontAssetMapJson["ContainerPath"] = iterator->containerPath;
		tempSelectedFontAssetMapJson["SelectedOption"] = WideMultiStringConverter.to_bytes(iterator->selectedOption);
		tempSelectedFontAssetMapJson["UseContainerPath"] = iterator->useContainerPath;
		exportJson["SelectedFontAssetMapList"].append(tempSelectedFontAssetMapJson);
	}

	return exportJson;
}

Json::Value _cdecl GetPacherConfigJson() {
	Json::Value patcherConfigJson;
	for (vector<FontAssetMap>::iterator iterator = selectedFontAssetMapList.begin();
		iterator != selectedFontAssetMapList.end(); iterator++) {
		Json::Value tempJson;
		tempJson["AssetsName"] = iterator->assetsName;
		tempJson["AssetName"] = iterator->assetName;
		tempJson["ContainerPath"] = iterator->containerPath;
		tempJson["SelectedOption"] = WideMultiStringConverter.to_bytes(iterator->selectedOption);
		tempJson["UseContainerPath"] = iterator->useContainerPath;
		patcherConfigJson.append(tempJson);
	}
	return patcherConfigJson;
}

bool _cdecl CopyBuildFileToBuildFolder(wstring FontPluginRelativePath, wstring targetPath) {
	CopyFileW((FontPluginRelativePath + L"TMPFontAsset\\").c_str(), (targetPath + L"TMPFontAsset").c_str(), false);
	return true;
}

Json::Value PatcherConfigGlobal;
bool _cdecl SetPacherConfigJson(Json::Value patcherConfig) {
	PatcherConfigGlobal = patcherConfig;
	return true;
}

AssetsReplacer* ReplaceMaterial(string assetsName, AssetsFileTable* assetsFileTable, AssetFileInfoEx* assetFileInfoEx, AssetTypeInstance* assetTypeInstance, float _TextureHeight, float _TextureWidth) {
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
	return UnityL10nToolAPIGlobal->makeAssetsReplacer(assetsName, assetsFileTable, assetFileInfoEx, assetTypeInstance);
}

AssetsReplacer* ReplaceAtlas(string assetsname, AssetsFileTable* assetsFileTable, AssetFileInfoEx* assetFileInfoEx, AssetTypeInstance* assetTypeInstance, int m_CompleteImageSize, string atlasPath, int m_Width, int m_Height) {
	AssetTypeValueField* assetTypeValueField = assetTypeInstance->GetBaseField();
	assetTypeInstance->GetBaseField()->Get("m_Width")->GetValue()->Set(new INT32(m_Width));
	assetTypeInstance->GetBaseField()->Get("m_Height")->GetValue()->Set(new INT32(m_Height));
	assetTypeInstance->GetBaseField()->Get("m_CompleteImageSize")->GetValue()->Set(new INT32(m_CompleteImageSize));
	assetTypeInstance->GetBaseField()->Get("m_StreamData")->Get("offset")->GetValue()->Set(new UINT32(0));
	assetTypeInstance->GetBaseField()->Get("m_StreamData")->Get("size")->GetValue()->Set(new UINT32(m_CompleteImageSize));
	assetTypeInstance->GetBaseField()->Get("m_StreamData")->Get("path")->GetValue()->Set((void*)(new string(atlasPath))->c_str());
	return UnityL10nToolAPIGlobal->makeAssetsReplacer(assetsname, assetsFileTable, assetFileInfoEx, assetTypeInstance);
}

map<string, string> copyList;
map<string, vector<AssetsReplacer*>> _cdecl GetPatcherAssetReplacer() {
	map<string, vector<AssetsReplacer*>> replacers;
	for (Json::ArrayIndex i = 0; i < PatcherConfigGlobal.size(); i++) {
		Json::Value assetLogicalReplacerJson = PatcherConfigGlobal[i];
		string assetsName = "";
		string assetName = "";
		string containerPath = "";
		string selectedOption = "";
		bool useContainerPath = false;

		if (assetLogicalReplacerJson["AssetsName"].isString()) {
			assetsName = assetLogicalReplacerJson["AssetsName"].asString();
		}
		if (assetLogicalReplacerJson["AssetName"].isString()) {
			assetName = assetLogicalReplacerJson["AssetName"].asString();
		}
		if (assetLogicalReplacerJson["ContainerPath"].isString()) {
			containerPath = assetLogicalReplacerJson["ContainerPath"].asString();
		}
		if (assetLogicalReplacerJson["SelectedOption"].isString()) {
			selectedOption = assetLogicalReplacerJson["SelectedOption"].asString();
		}
		if (assetLogicalReplacerJson["UseContainerPath"].isBool()) {
			useContainerPath = assetLogicalReplacerJson["UseContainerPath"].asBool();
		}
		Json::Value selectedOptionJson;
		if (!selectedOption.empty()) {
			string selectedOptionStr = readFile2(FontPluginInfoGlobal->relativePluginPath + L"TMPFontAsset\\" + WideMultiStringConverter.from_bytes(selectedOption) + L".json");
			JsonReader.parse(selectedOptionStr, selectedOptionJson);
		}

		if (((useContainerPath && !containerPath.empty())
			|| (!useContainerPath && !assetsName.empty() && !assetName.empty()))
			&& (selectedOptionJson.getMemberNames().size() != 0)) {
			string monoPath = "";
			// for material
			float _TextureHeight;
			float _TextureWidth;
			// for atlas
			int m_Width = 0;
			int m_Height = 0;
			int m_CompleteImageSize;
			string atlasPath = "";
			if (selectedOptionJson["monoPath"].isString()) {
				monoPath = selectedOptionJson["monoPath"].asString();
			}
			if (selectedOptionJson["_TextureHeight"].isDouble()) {
				_TextureHeight = selectedOptionJson["_TextureHeight"].asFloat();
			}
			if (selectedOptionJson["_TextureWidth"].isDouble()) {
				_TextureWidth = selectedOptionJson["_TextureWidth"].asFloat();
			}
			if (selectedOptionJson["m_Width"].isInt()) {
				m_Width = selectedOptionJson["m_Width"].asInt();
			}
			if (selectedOptionJson["m_Height"].isInt()) {
				m_Height = selectedOptionJson["m_Height"].asInt();
			}
			if (selectedOptionJson["m_CompleteImageSize"].isInt()) {
				m_CompleteImageSize = selectedOptionJson["m_CompleteImageSize"].asInt();
			}
			if (selectedOptionJson["atlasPath"].isString()) {
				atlasPath = selectedOptionJson["atlasPath"].asString();
			}

			string assetsFileName;
			AssetsFileTable* assetsFileTable;
			AssetFileInfoEx* assetFileInfoEx;
			AssetTypeInstance* assetTypeInstance;
			AssetTypeValueField* baseAssetTypeValueField;

			//INT64 PathId;
			try {
				if (useContainerPath) {
					map<string, pair<INT32, INT64>>::const_iterator FilePathIdIterator = UnityL10nToolAPIGlobal->FindFileIDPathIDFromContainerPath->find(containerPath);
					if (FilePathIdIterator == UnityL10nToolAPIGlobal->FindFileIDPathIDFromContainerPath->end()) {
						continue;
					}
					map<INT32, string>::const_iterator assetsFileNameIterator = UnityL10nToolAPIGlobal->FindAssetsNameFromPathIDOfContainerPath->find(FilePathIdIterator->second.first);
					if (assetsFileNameIterator == UnityL10nToolAPIGlobal->FindAssetsNameFromPathIDOfContainerPath->end()) {
						continue;
					}
					assetsFileName = assetsFileNameIterator->second;
					map<string, AssetsFileTable*>::const_iterator assetsFileTableIterator = UnityL10nToolAPIGlobal->FindAssetsFileTablesFromAssetsName->find(assetsFileName);
					if (assetsFileTableIterator == UnityL10nToolAPIGlobal->FindAssetsFileTablesFromAssetsName->end()) {
						continue;
					}
					assetsFileTable = assetsFileTableIterator->second;
					assetFileInfoEx = assetsFileTable->getAssetInfo(FilePathIdIterator->second.second);
				}
				else {
					assetsFileName = assetsName;
					map<string, AssetsFileTable*>::const_iterator assetsFileTableIterator = UnityL10nToolAPIGlobal->FindAssetsFileTablesFromAssetsName->find(assetsFileName);
					if (assetsFileTableIterator == UnityL10nToolAPIGlobal->FindAssetsFileTablesFromAssetsName->end()) {
						continue;
					}
					assetsFileTable = assetsFileTableIterator->second;
					unsigned int assetIndex = UnityL10nToolAPIGlobal->FindAssetIndexFromName(assetsFileTable, assetName);
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
			assetTypeInstance = UnityL10nToolAPIGlobal->GetDetailAssetTypeInstanceFromAssetFileInfoEx(assetsFileTable, assetFileInfoEx);
			baseAssetTypeValueField = assetTypeInstance->GetBaseField();
			if (baseAssetTypeValueField->IsDummy())
			{
				continue;
			}
			string modifyStr = readFile2(FontPluginInfoGlobal->relativePluginPath + L"TMPFontAsset\\" + WideMultiStringConverter.from_bytes(monoPath));
			Json::Value modifyJson;
			JsonReader.parse(modifyStr, modifyJson);
			AssetsReplacer* monoAssetsReplacer = UnityL10nToolAPIGlobal->makeAssetsReplacer(assetsFileName, assetsFileTable, assetFileInfoEx, assetTypeInstance, modifyJson);

			INT64 materialPathId = baseAssetTypeValueField->Get("material")->Get("m_PathID")->GetValue()->AsInt64();
			AssetFileInfoEx* materialAssetFileInfoEx = assetsFileTable->getAssetInfo(materialPathId);
			AssetTypeInstance* materialAssetTypeInstance = UnityL10nToolAPIGlobal->GetDetailAssetTypeInstanceFromAssetFileInfoEx(assetsFileTable, materialAssetFileInfoEx);
			AssetsReplacer* materialAssetsReplacer = ReplaceMaterial(assetsFileName, assetsFileTable, materialAssetFileInfoEx, materialAssetTypeInstance, _TextureHeight, _TextureWidth);

			INT64 atlasPathId = baseAssetTypeValueField->Get("atlas")->Get("m_PathID")->GetValue()->AsInt64();
			AssetFileInfoEx* atlasAssetFileInfoEx = assetsFileTable->getAssetInfo(atlasPathId);
			AssetTypeInstance* atlasAssetTypeInstance = UnityL10nToolAPIGlobal->GetDetailAssetTypeInstanceFromAssetFileInfoEx(assetsFileTable, atlasAssetFileInfoEx);
			AssetsReplacer* atlasAssetsReplacer = ReplaceAtlas(assetsFileName, assetsFileTable, atlasAssetFileInfoEx, atlasAssetTypeInstance, m_CompleteImageSize, atlasPath, m_Width, m_Height);

			replacers[assetsFileName].push_back(monoAssetsReplacer);
			replacers[assetsFileName].push_back(materialAssetsReplacer);
			replacers[assetsFileName].push_back(atlasAssetsReplacer);
			copyList.insert(pair<string, string>(atlasPath, atlasPath));
		}
	}
	return replacers;
}

bool _cdecl CopyResourceFileToGameFolder(wstring FontPluginRelativePath, wstring targetPath) {
	for (map<string, string>::iterator iterator = copyList.begin(); iterator != copyList.end(); iterator++) {
		CopyFileW((FontPluginRelativePath + L"TMPFontAsset\\" + WideMultiStringConverter.from_bytes(iterator->first)).c_str(),
			(targetPath + L"TMPFontAsset\\" + WideMultiStringConverter.from_bytes(iterator->second)).c_str(), false);
	}
	//CopyFileW((FontPluginRelativePath + L"TMPFontAsset\\").c_str(), (targetPath + L"TMPFontAsset").c_str(), false);
	return true;
}

bool _cdecl GetFontPluginInfo(UnityL10nToolAPI* unityL10nToolAPI, FontPluginInfo* fontPluginInfo) {
	UnityL10nToolAPIGlobal = unityL10nToolAPI;
	FontPluginInfoGlobal = fontPluginInfo;
	wcsncpy_s(fontPluginInfo->FontPluginName, L"TMPFontAsset", 12);
	fontPluginInfo->SetProjectConfigJson = SetProjectConfigJson;
	fontPluginInfo->GetPluginSupportAssetMap = GetPluginSupportAssetMap;
	fontPluginInfo->SetPluginAssetMap = SetPluginAssetMap;
	fontPluginInfo->GetProjectConfigJson = GetProjectConfigJson;
	fontPluginInfo->GetPacherConfigJson = GetPacherConfigJson;
	fontPluginInfo->CopyBuildFileToBuildFolder = CopyBuildFileToBuildFolder;

	fontPluginInfo->SetPacherConfigJson = SetPacherConfigJson;
	fontPluginInfo->GetPatcherAssetReplacer = GetPatcherAssetReplacer;
	fontPluginInfo->CopyResourceFileToGameFolder = CopyResourceFileToGameFolder;

	string optionsJsonStr = readFile2(fontPluginInfo->relativePluginPath + L"TMPFontAsset\\Options.json");
	JsonReader.parse(optionsJsonStr, OptionsJson);

	if (OptionsJson.getMemberNames().size() != 0) {
		vector<string> tempOptionsList = OptionsJson.getMemberNames();
		for (vector<string>::iterator iterator = tempOptionsList.begin();
			iterator != tempOptionsList.end(); iterator++) {
			OptionsList.push_back(WideMultiStringConverter.from_bytes(*iterator));
		}
	}
	else {
		//장차 없에고, 옵션이 없으면 오류를 내는 쪽으로
		OptionsList.push_back(L"Open Sans");
		OptionsList.push_back(L"Noto Sans CJK KR");
		OptionsList.push_back(L"나눔바른고딕");
	}
	return true;
}