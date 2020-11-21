#include "../Common/Common.hpp"
#include "../Common/Font.hpp"

#include "rapidjson/document.h"

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
    string jsonname = "config.json";
    if (NArg > 1) {
        jsonname = Args[1];
        fprintf(stdout, "%s specified.\n", Args[1]);
    };

    // dirty open a file in main
    ifstream in(jsonname, ios::in);
    if (!in)
    {
        cerr << "Error opening file" << endl;
        return EXIT_FAILURE;
    }
    istreambuf_iterator<char> beg(in), end;
    string json(beg, end);
    in.close();

    // shitty json lib here
    rapidjson::Document d;
    d.Parse(json.c_str());

    if (!d.HasMember("filename")) {
        fprintf(stderr, "Invalid json.\n");
        fprintf(stderr,
            "\n"
            "Create DC6 and TBL according to given font and codepoint range\n"
            "\n"
            "Usage: %s **params moved to json**\n"
            "Construct DC6 and TBL file using the specified font face and point size.\n"
            "Note: The font must be supported by FreeType.\n"
            "Use null as the palatte to encode as grayscale images.\n",
            Args[0]
        );
        return EXIT_FAILURE;
    }
  Font Fnt;

  auto FacePath = d["path"].GetString();
  uint16_t Size = d["size"].GetInt();
  auto HeightConstant = d["leadingfactor"].GetInt();
  auto LnSpacingOff = d["LeadingOffset"].GetInt();
  auto CapHeight = d["CapHeight"].GetInt();
  auto OriginOffset = d["OriginOffset"].GetInt();
  auto PalPath = d["pal"].GetString();
  auto Dc6Path = d["dc6name"].GetString();
  auto TblPath = d["tblname"].GetString();

  // Build a list with your free-to-waste RAM
  vector<uint16_t> glyphlist;

  // Juse werk snippet for parsing ranges
  const rapidjson::Value& h = d["ranges"];
  for (rapidjson::Value::ConstValueIterator v_iter = h.Begin();
      v_iter != h.End(); ++v_iter)
  {
      const rapidjson::Value& field = *v_iter;
      for (rapidjson::Value::ConstMemberIterator m_iter = field.MemberBegin();
          m_iter != field.MemberEnd(); ++m_iter)
      {
          auto start = m_iter->value[0].GetInt();
          auto end = m_iter->value[1].GetInt();
          for (auto i = (unsigned) start; i <= (unsigned) end; i++)
              glyphlist.push_back(i);
          break;
      }
  }

  auto boolaa = d["aa"].GetBool(); // currently global AA
  //int bg = d["bgColor"][0].GetInt();

  printf("Preparing glyphs...\n");
  Fnt.Size = Size;
  Fnt.HeightConstant = HeightConstant;
  Fnt.LnSpacingOff = LnSpacingOff;
  Fnt.CapHeight = CapHeight;
  Fnt.OriginOffset = OriginOffset;
  Fnt.Faces.emplace_back(FacePath);
  for (auto it = glyphlist.cbegin(); it != glyphlist.cend(); it++) {
    uint16_t Ch = *it;
    auto& G = Fnt.Glyphs[Ch];
    G.reset(new FontGlyph);
    G->Char = Ch;
    G->AntiAliasing = boolaa;
    G->Size = Size;
    G->FaceIdx = 0;
    G->HasBmp = false;
  }
  printf("Rendering glyphs...\n");
  Fnt.RenderGlyphs();
  printf("Reading palette...\n");
  Palette Pal;
  Pal.ReadDat(PalPath);
  printf("Dumping font...\n");
  Sprite Spr;
  FontTable Tbl;
  Fnt.Dump(Spr, Tbl);
  printf("Saving DC6...\n");
  Spr.SaveDc6(Dc6Path, Pal);
  printf("Saving TBL...\n");
  Tbl.SaveTbl(TblPath);
  printf("All done\n");
  return 0;
}