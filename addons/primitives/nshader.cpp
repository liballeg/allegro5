/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      This program is used to pre-compile some dummy shaders from
 *      nshader.fx to be used in directx_shaders.c file. You will need 
 *      fxc.exe and vsa.exe to run this program, which can be found in
 *      the April 2008 DirectX SDK (it has been removed in the newer SDK
 *      versions).
 * 
 *      Usage: Just run this program, and copy the output (tables.h)
 *      into the appropriate location in directx_shaders.c.
 * 
 *      Note, that this is not supposed to be compiled into the library.
 *      Compile this manually if needed.
 *
 *      By Michał Cichoń.
 *
 *      See readme.txt for copyright information.
 */

# include <iostream>
# include <algorithm>
# include <fstream>
# include <cstdlib>
# include <vector>
# include <string>
# include <sstream>
# include <cstddef>
# include <iomanip>

using namespace std;

typedef unsigned char byte_t;

typedef vector<byte_t> byte_vector_t;

struct technique_t
{
  string        name;
  string        shader;
  byte_vector_t binary;
};

typedef vector<technique_t> technique_vector_t;

int compile(const string& source, const string& dest)
{
  return system(("fxc /nologo /Tfx_2_0 /Op /O3 /Fc " + dest + " " + source + " > nul").c_str());
}

int assemble(const string& source, const string& dest)
{
  return system(("vsa /nologo /Fo " + dest + " " + source + " > nul").c_str());
}

bool extract_shader(technique_t& technique, istream& stream)
{
  technique.shader.clear();

  bool inside = false;
  bool record = false;

  string line;
  while (!getline(stream, line).fail())
  {
    if (!inside && !record)
    {
      string::size_type pos = line.find_first_of("asm");
      if (pos == string::npos || line.compare(pos, 3, "asm") != 0)
        continue;

      // strip everything to including "asm" keyword
      line = line.substr(pos + 3);

      inside = true;
    }

    if (inside && !record)
    {
      string::size_type pos = line.find_first_of("{");
      if (pos == string::npos)
        continue;

      record = true;

      continue;
    }

    if (inside && record)
    {
      string::size_type pos = line.find_first_of("}");
      if (pos != string::npos)
        return true;

      string::size_type non_white = line.find_first_not_of(" ");
      if (non_white != string::npos)
        technique.shader += line.substr(non_white) + "\n";
      else
        technique.shader += "\n";
    }
  }

  return false;
}

int extract_techniques(technique_vector_t& techniques, const string& filename)
{
  techniques.clear();

  ifstream file(filename.c_str());
  if (!file)
    return 0;

  technique_t technique;

  string line;
  while (!getline(file, line).fail())
  {
    string::size_type pos = line.find_first_of("technique");
    if (pos != 0 || line.compare(pos, 9, "technique") != 0)
      continue;

    technique.name = line.substr(10);
    if (technique.name.empty())
    {
      cerr << "  Unnamed technique. Skipping..." << endl;
      continue;
    }

    cout << "  Found: " << technique.name << endl;
    if (!extract_shader(technique, file))
    {
      cerr << "    Failed! Skipping..." << endl;
      continue;
    }

    techniques.push_back(technique);
  }

  return (int)techniques.size();
}

int assemble_technique(technique_t& technique)
{
  const string source = "vshader.vs";
  const string dest   = "vshader.vso";

  {
    ofstream file(source.c_str());
    file << technique.shader;
  }

  int result = assemble(source, dest);
  if (result)
    return result;

  ifstream file(dest.c_str(), ios::in | ios::binary);
  if (!file)
  {
    _unlink(dest.c_str());
    _unlink(source.c_str());

    return 1;
  }

  file.seekg(0, ios::end);
  technique.binary.resize(file.tellg());
  file.seekg(0, ios::beg);
  file.read((char*)&technique.binary.front(), technique.binary.size());
  file.close();

  _unlink(dest.c_str());
  _unlink(source.c_str());

  return 0;
}

int assemble_techniques(technique_vector_t& techniques)
{
  int count = 0;

  technique_vector_t::iterator it, it_end = techniques.end();
  for (it = techniques.begin(); it != it_end; ++it)
  {
    technique_t& technique = *it;

    cout << "  Processing: " << technique.name << endl;
    int assembled = assemble_technique(technique);
    if (assembled)
    {
      cerr << "    Failed." << endl;
      continue;
    }

    ++count;
  }

  return count;
}

char toupper2(char c)
{
   return toupper(c);
}

int generate_table(ostream& output, technique_t& technique)
{
  const int bytes_per_line = 16;

  string::size_type pos = 0;
  string name = technique.name, line;

  while ((pos = name.find_first_of('_', pos)) != string::npos)
    name[pos] = ' ';

  transform(name.begin(), name.end(), name.begin(), toupper2);

  output << "/*" << endl;
  output << "   " << name << endl << endl;

  istringstream shader(technique.shader);
  while (!getline(shader, line).fail())
    output << "   " << line << endl;
  output << "*/" << endl;

  output << "static const uint8_t _al_vs_" << technique.name << "[] = {" << endl;

  for (size_t i = 0; i < technique.binary.size(); ++i)
  {
    if ((i % bytes_per_line) == 0)
      output << "   ";

    output << "0x" << setw(2) << setfill('0') << hex << (int)technique.binary[i];

    if (i != technique.binary.size() - 1)
    {
      if ((i % bytes_per_line) == (bytes_per_line - 1))
        output << "," << endl;
      else
        output << ", ";
    }
  }
  output << endl;

  output.unsetf(ios::hex);

  output << "};" << endl;

  return 0;
}

int generate_tables(ostream& output, technique_vector_t& techniques)
{
  int count = 0;

  technique_vector_t::iterator it, it_end = techniques.end();
  for (it = techniques.begin(); it != it_end; ++it)
  {
    technique_t& technique = *it;

    cout << "  Processing: " << technique.name << endl;
    int generated = generate_table(output, technique);
    if (generated)
    {
      cerr << "    Failed." << endl;
      continue;
    }

    output << endl;

    ++count;
  }

  return count;
}

int main()
{
  const string source   = "nshader.fx";
  const string compiled = "nshader.fxc";
  const string output   = "tables.h";

  cout << "Compiling NULL shader..." << endl;
  if (int result = compile(source, compiled))
  {
    cout << "  Failed!" << endl;
    return result;
  }
  cout << "Done." << endl << endl;

  cout << "Extracting techniques..." << endl;
  technique_vector_t techniques;
  int found = extract_techniques(techniques, compiled);
  if (found)
    cout << "Done. " << found << " technique(s) extracted." << endl << endl;
  else
    cout << "Done. No techniques extracted." << endl << endl;

  _unlink(compiled.c_str());

  if (found == 0)
    return 0;

  cout << "Assembling techniques..." << endl;
  int assembled = assemble_techniques(techniques);
  if (assembled)
    cout << "Done. " << assembled << " technique(s) assembled." << endl << endl;
  else
    cout << "Done. No assembled techniques." << endl << endl;

  if (assembled == 0)
    return 0;

  cout << "Generating tables..." << endl;

  ofstream output_file(output.c_str());
  int tables = generate_tables(output_file, techniques);
  cout << "Done. " << tables << " table(s) generated." << endl << endl;

  return 0;
}
