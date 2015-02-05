#include <iostream>
#include <elfio/elfio.hpp>

/* objdump can't dump oat files because .text sections seems
 * wrongly ordered (probably Android use a different standard).
 * In order to get a objdumpable file we must reorder bytes.
 *
 *  Wrong order ---- Correct order
 *  3                1
 *  4                2
 *  1                3
 *  2                4
 *
 */

using namespace ELFIO;

void initialize_elf_writer(elfio *writer);
section* append_text_section(elfio *writer);

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cout << "usage: oatrev <elf_file>" << std::endl << "   output is <elf_file>.rev" << std::endl;
        return 1;
    }
    
    elfio reader;
    elfio writer;
    
    if (!reader.load(argv[1])) {
        std::cout << "failed to load input file (is a elf file?)" << argv[1] << std::endl;
        return 1;
    }

    Elf_Half sec_num = reader.sections.size();
    for (auto section : reader.sections) {
      if (section->get_name() == ".text") {
        // .text found, initialize some variables.
        initialize_elf_writer(&writer);
        auto size = section->get_size();
        auto text_sec = append_text_section(&writer);

        // create data pointers.
        auto torev = section->get_data();
        auto rev = new char[size];
        
        // fill reordered .text section.
        for (auto count = 0; count<size; count+=4) {
          rev[count]   = torev[count+2];
          rev[count+1] = torev[count+3];
          rev[count+2] = torev[count];
          rev[count+3] = torev[count+1];
        }
        
        // set .text section data.
        text_sec->set_data(rev, size);

        // save new elf file.
        writer.save(std::string(argv[1])+".rev");

        // free temp buffer, (cosmetical because we are about to exit).
        delete rev;

        return 0;
      }
    }

    return 1;
}

void initialize_elf_writer(elfio *writer) {
  writer->create(ELFCLASS32, ELFDATA2LSB);
  writer->set_os_abi(ELFOSABI_LINUX);
  writer->set_type(ET_EXEC);
  writer->set_machine(EM_ARM);
}

section* append_text_section(elfio *writer) {
  auto text_sec = writer->sections.add(".text");
  text_sec->set_type(SHT_PROGBITS);
  text_sec->set_flags(SHF_ALLOC | SHF_EXECINSTR);
  text_sec->set_addr_align(0x2);

  return text_sec;
}
