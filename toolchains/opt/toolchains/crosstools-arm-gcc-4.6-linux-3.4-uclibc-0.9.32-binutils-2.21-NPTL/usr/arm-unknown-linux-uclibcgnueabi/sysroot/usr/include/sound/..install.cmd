cmd_/home/dslsqa/rel1.2/src/buildroot-2011.11/output/toolchain/linux/include/sound/.install := perl scripts/headers_install.pl /home/dslsqa/rel1.2/src/buildroot-2011.11/output/toolchain/linux-3.4/include/sound /home/dslsqa/rel1.2/src/buildroot-2011.11/output/toolchain/linux/include/sound arm asequencer.h asound.h asound_fm.h compress_offload.h compress_params.h emu10k1.h hdsp.h hdspm.h sb16_csp.h sfnt_info.h; perl scripts/headers_install.pl /home/dslsqa/rel1.2/src/buildroot-2011.11/output/toolchain/linux-3.4/include/sound /home/dslsqa/rel1.2/src/buildroot-2011.11/output/toolchain/linux/include/sound arm ; perl scripts/headers_install.pl /home/dslsqa/rel1.2/src/buildroot-2011.11/output/toolchain/linux-3.4/include/generated/sound /home/dslsqa/rel1.2/src/buildroot-2011.11/output/toolchain/linux/include/sound arm ; for F in ; do echo "\#include <asm-generic/$$F>" > /home/dslsqa/rel1.2/src/buildroot-2011.11/output/toolchain/linux/include/sound/$$F; done; touch /home/dslsqa/rel1.2/src/buildroot-2011.11/output/toolchain/linux/include/sound/.install
