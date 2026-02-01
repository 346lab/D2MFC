#include "../Common/Common.hpp"
#include "../Common/Font.hpp"

#include <yaml-cpp/yaml.h>

#include <vector>
#include <iostream>
#include <fstream>

template<class T>
T Parse(const char* S, const char* Desc) {
  uintmax_t Res{};
  while (*S) {
    if (!isdigit(*S))
      Abort("Failed to parse %s", Desc);
    auto Tmp = Res * 10 + (*S++ & 0xf);
    if (Tmp < Res)
      Abort("Integer overflow when parsing %s", Desc);
    Res = Tmp;
  }
  return Cast<T>(Res, "The %s is too large (%" PRIuMAX ")", Desc, Res);
}

int main(int NArg, char* Args[]) {
    string cfgname = "config.yaml";
    if (NArg > 1) {
        cfgname = Args[1];
        fprintf(stdout, "%s specified.\n", Args[1]);
    };

    // open config file
    YAML::Node d;
    try {
      d = YAML::LoadFile(cfgname);
    }
    catch (const YAML::BadFile& e) {
      cerr << "Error opening config file: " << cfgname << endl;
      return EXIT_FAILURE;
    }

    if (!d["filename"]) {
        fprintf(stderr, "Invalid config.\n");
        fprintf(stderr,
            "\n"
            "Create DC6 and TBL according to given font and codepoint range\n"
            "\n"
            "Usage: %s **params moved to config.yaml**\n"
            "Construct DC6 and TBL file using the specified font face and point size.\n"
            "Note: The font must be supported by FreeType.\n"
            "Use null as the palatte to encode as grayscale images.\n",
            Args[0]
        );
        return EXIT_FAILURE;
    }
  Font Fnt;

  // Top-level/legacy values
  auto baseName = d["filename"].as<string>();
  auto heightConstant = d["leadingFactor"] ? d["leadingFactor"].as<int>() : Fnt.HeightConstant;
  auto lnSpacingOff = d["leadingOffset"] ? d["leadingOffset"].as<int>() : Fnt.LnSpacingOff;
  auto capHeight = d["capHeight"] ? d["capHeight"].as<int>() : -1;
  auto capHeightOffset = d["capHeightOffset"] ? d["capHeightOffset"].as<int>() : Fnt.CapHeightOff;
  auto originOffset = d["originOffset"] ? d["originOffset"].as<int>() : 0;
  auto palPath = d["palette"] ? d["palette"].as<string>() : "static.pal";
  auto dc6Path = d["dc6name"] ? d["dc6name"].as<string>() : (baseName + string(".dc6"));
  auto tblPath = d["tblname"] ? d["tblname"].as<string>() : (baseName + string(".tbl"));
  int32_t globalDc6OffsetY = d["dc6OffsetY"] ? d["dc6OffsetY"].as<int>() : 0;
  int32_t descentPadding = Fnt.DescentPadding;
  if (d["descentPadding"] && !d["descentPadding"].IsNull()) descentPadding = d["descentPadding"].as<int>();
  int globalTblUnk = d["tblUnknownValue"] ? d["tblUnknownValue"].as<int>() : 0;
  auto doBrightnessShift = d["doBrightnessShift"] ? d["doBrightnessShift"].as<bool>() : false;
  auto brightnessShiftOffset = d["brightnessShiftOffset"] ? d["brightnessShiftOffset"].as<unsigned>() : 0;

  // Partition-based config (preferred)
  printf("Preparing glyphs...\n");
  bool firstPartition = true;
  const YAML::Node partsNode = d["partitonConfig"] ? d["partitonConfig"] : (d["partitionConfig"] ? d["partitionConfig"] : YAML::Node());
  if (partsNode) {
    for (auto it = partsNode.begin(); it != partsNode.end(); ++it) {
      auto p = *it;
      auto start = p["start"].as<int>();
      auto end = p["end"].as<int>();
      auto facePath = p["fontFace"].as<string>();
      auto Size = p["size"].as<int>();
      auto aa = p["aa"] ? p["aa"].as<bool>() : true;
      auto glyphColor = p["glyphColor"] ? p["glyphColor"].as<unsigned int>() : (d["glyphColor"] ? d["glyphColor"].as<unsigned int>() : 0xFFFFFFu);
      auto bgColor = p["bgColor"] ? p["bgColor"].as<unsigned int>() : (d["bgColor"] ? d["bgColor"].as<unsigned int>() : 0x000000u);
      auto tblTwo = p["tblUnknownValueTwo"] ? p["tblUnknownValueTwo"].as<uint8_t>() : (d["tblUnknownValueTwo"] ? d["tblUnknownValueTwo"].as<uint8_t>() : 1);
      auto invalidIndex = p["invalidGlyphIndex"] ? p["invalidGlyphIndex"].as<uint16_t>() : (d["invalidGlyphIndex"] ? d["invalidGlyphIndex"].as<uint16_t>() : 1);
      auto doOutlineGlyphs = p["doOutlineGlyphs"] ? p["doOutlineGlyphs"].as<bool>() : (d["doOutlineGlyphs"] ? d["doOutlineGlyphs"].as<bool>() : false);
      auto outlineColor = p["outlineColor"] ? p["outlineColor"].as<unsigned>() : (d["outlineColor"] ? d["outlineColor"].as<unsigned int>() : 0x010101u);

      // register face and get index
      int faceIdx = -1;
      for (size_t fi = 0; fi < Fnt.Faces.size(); ++fi)
        if (Fnt.Faces[fi] == facePath) { faceIdx = (int)fi; break; }
      if (faceIdx < 0) { Fnt.Faces.emplace_back(facePath); faceIdx = (int)Fnt.Faces.size() - 1; }

      if (firstPartition) { Fnt.Size = Size; firstPartition = false; }

      for (auto ch = start; ch <= end; ++ch) {
        auto& G = Fnt.Glyphs[ch];
        if (!G) { G.reset(new FontGlyph); G->Char = (uint16_t)ch; }
        G->AntiAliasing = aa;
        G->Size = Size;
        G->FaceIdx = faceIdx;
        G->HasBmp = false;
        G->UnkTwo = (uint8_t)tblTwo;
        G->FgCol = Pixel{ (uint8_t)((glyphColor >> 16) & 0xFF), (uint8_t)((glyphColor >> 8) & 0xFF), (uint8_t)(glyphColor & 0xFF) };
        G->BgCol = Pixel{ (uint8_t)((bgColor >> 16) & 0xFF), (uint8_t)((bgColor >> 8) & 0xFF), (uint8_t)(bgColor & 0xFF) };
        G->InvalidGlyphIndex = invalidIndex;
        G->DoOutlineGlyphs = doOutlineGlyphs;
        G->OutlineColor = outlineColor;
      }
    }
  }
  // Legacy ranges support
  else if (d["ranges"]) {
    auto FacePath = d["path"].as<string>();
    auto Size = d["size"].as<int>();
    Fnt.Faces.emplace_back(FacePath);
    Fnt.Size = Size;
    for (auto it = d["ranges"].begin(); it != d["ranges"].end(); ++it) {
      auto node = *it;
      if (node["range"]) {
        auto start = node["range"][0].as<int>();
        auto end = node["range"][1].as<int>();
        for (auto ch = start; ch <= end; ++ch) {
          auto& G = Fnt.Glyphs[ch];
          G.reset(new FontGlyph);
          G->Char = (uint16_t)ch;
          G->AntiAliasing = d["aa"] ? d["aa"].as<bool>() : true;
          G->Size = Size;
          G->FaceIdx = 0;
          G->HasBmp = false;
        }
      }
    }
  }

  // apply top-level font params
  Fnt.HeightConstant = heightConstant;
  Fnt.LnSpacingOff = lnSpacingOff;
  Fnt.CapHeight = capHeight;
  Fnt.CapHeightOff = capHeightOffset;
  Fnt.OriginOffset = originOffset;
  Fnt.DescentPadding = descentPadding;
  Fnt.UnkHZ = globalTblUnk;
  Fnt.DoBrightnessShift = doBrightnessShift;
  Fnt.BrightnessShiftOffset = brightnessShiftOffset;
  printf("Reading palette...\n");
  Palette Pal;
  Pal.ReadDat(palPath.data());
  printf("Rendering glyphs...\n");
  Fnt.RenderGlyphs(&Pal);
  printf("Dumping font...\n");
  Sprite Spr;
  FontTable Tbl;
  Fnt.Dump(Spr, Tbl);
  printf("Saving DC6...\n");
  Spr.SaveDc6(dc6Path.data(), Pal, globalDc6OffsetY);
  printf("Saving TBL...\n");
  Tbl.SaveTbl(tblPath.data());
  printf("All done\n");
  return 0;
}