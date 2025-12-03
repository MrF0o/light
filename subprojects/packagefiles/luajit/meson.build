project('luajit', 'c', version: '2.1')

cc = meson.get_compiler('c')

# LuaJIT VM core
vm_sources = files(
  'src/lj_gc.c',
  'src/lj_err.c',
  'src/lj_char.c',
  'src/lj_bc.c',
  'src/lj_obj.c',
  'src/lj_buf.c',
  'src/lj_str.c',
  'src/lj_tab.c',
  'src/lj_func.c',
  'src/lj_udata.c',
  'src/lj_meta.c',
  'src/lj_debug.c',
  'src/lj_prng.c',
  'src/lj_state.c',
  'src/lj_dispatch.c',
  'src/lj_vmevent.c',
  'src/lj_vmmath.c',
  'src/lj_strscan.c',
  'src/lj_strfmt.c',
  'src/lj_strfmt_num.c',
  'src/lj_serialize.c',
  'src/lj_api.c',
  'src/lj_profile.c',
  'src/lj_lex.c',
  'src/lj_parse.c',
  'src/lj_bcread.c',
  'src/lj_bcwrite.c',
  'src/lj_load.c',
  'src/lj_ir.c',
  'src/lj_opt_mem.c',
  'src/lj_opt_fold.c',
  'src/lj_opt_narrow.c',
  'src/lj_opt_dce.c',
  'src/lj_opt_loop.c',
  'src/lj_opt_split.c',
  'src/lj_opt_sink.c',
  'src/lj_mcode.c',
  'src/lj_snap.c',
  'src/lj_record.c',
  'src/lj_crecord.c',
  'src/lj_ffrecord.c',
  'src/lj_asm.c',
  'src/lj_trace.c',
  'src/lj_gdbjit.c',
  'src/lj_ctype.c',
  'src/lj_cdata.c',
  'src/lj_cconv.c',
  'src/lj_ccall.c',
  'src/lj_ccallback.c',
  'src/lj_carith.c',
  'src/lj_clib.c',
  'src/lj_cparse.c',
  'src/lj_lib.c',
  'src/lj_alloc.c',
  'src/lib_aux.c',
  'src/lib_base.c',
  'src/lib_math.c',
  'src/lib_bit.c',
  'src/lib_string.c',
  'src/lib_table.c',
  'src/lib_io.c',
  'src/lib_os.c',
  'src/lib_package.c',
  'src/lib_debug.c',
  'src/lib_jit.c',
  'src/lib_ffi.c',
  'src/lib_buffer.c',
)

# Build minilua first (needed for generating buildvm)
minilua = executable('minilua',
  'src/host/minilua.c',
  native: true,
  c_args: ['-DLUAI_MAXCSTACK=65536'],
)

# Generate buildvm_arch.h using DynASM
dynasm_lua = find_program('dynasm/dynasm.lua')
buildvm_arch_h = custom_target('buildvm_arch.h',
  output: 'buildvm_arch.h',
  input: 'src/vm_x64.dasc',
  command: [minilua, dynasm_lua, '-LN', '-o', '@OUTPUT@', '@INPUT@'],
)

# Build buildvm
buildvm_sources = files(
  'src/host/buildvm.c',
  'src/host/buildvm_asm.c',
  'src/host/buildvm_peobj.c',
  'src/host/buildvm_lib.c',
  'src/host/buildvm_fold.c',
)

buildvm = executable('buildvm',
  buildvm_sources,
  buildvm_arch_h,
  native: true,
  include_directories: include_directories('src'),
  c_args: ['-DLUAJIT_TARGET=LUAJIT_ARCH_X64'],
)

# Generate lj_vm.S using buildvm
lj_vm_s = custom_target('lj_vm.S',
  output: 'lj_vm.S',
  command: [buildvm, '-m', 'elfasm', '-o', '@OUTPUT@'],
)

# Generate headers using buildvm
lj_bcdef_h = custom_target('lj_bcdef.h',
  output: 'lj_bcdef.h',
  input: 'src/lj_bc.h',
  command: [buildvm, '-m', 'bcdef', '-o', '@OUTPUT@', '@INPUT@'],
)

lj_ffdef_h = custom_target('lj_ffdef.h',
  output: 'lj_ffdef.h',
  input: files('src/lib_base.c', 'src/lib_math.c', 'src/lib_bit.c', 'src/lib_string.c', 
               'src/lib_table.c', 'src/lib_io.c', 'src/lib_os.c', 'src/lib_package.c',
               'src/lib_debug.c', 'src/lib_jit.c', 'src/lib_ffi.c', 'src/lib_buffer.c'),
  command: [buildvm, '-m', 'ffdef', '-o', '@OUTPUT@', '@INPUT@'],
)

lj_libdef_h = custom_target('lj_libdef.h',
  output: 'lj_libdef.h',
  input: files('src/lib_base.c', 'src/lib_math.c', 'src/lib_bit.c', 'src/lib_string.c',
               'src/lib_table.c', 'src/lib_io.c', 'src/lib_os.c', 'src/lib_package.c',
               'src/lib_debug.c', 'src/lib_jit.c', 'src/lib_ffi.c', 'src/lib_buffer.c'),
  command: [buildvm, '-m', 'libdef', '-o', '@OUTPUT@', '@INPUT@'],
)

lj_recdef_h = custom_target('lj_recdef.h',
  output: 'lj_recdef.h',
  input: files('src/lj_ffrecord.c'),
  command: [buildvm, '-m', 'recdef', '-o', '@OUTPUT@', '@INPUT@'],
)

lj_folddef_h = custom_target('lj_folddef.h',
  output: 'lj_folddef.h',
  input: 'src/lj_opt_fold.c',
  command: [buildvm, '-m', 'folddef', '-o', '@OUTPUT@', '@INPUT@'],
)

# LuaJIT compile flags
luajit_cflags = [
  '-DLUAJIT_ENABLE_LUA52COMPAT',
  '-DLUAJIT_ENABLE_GC64',
  '-U_FORTIFY_SOURCE',
]

# Find dependencies
libm = cc.find_library('m', required: false)
libdl = cc.find_library('dl', required: false)

# Build the static library
luajit_lib = static_library('luajit_main',
  vm_sources,
  lj_vm_s,
  lj_bcdef_h,
  lj_ffdef_h,
  lj_libdef_h,
  lj_recdef_h,
  lj_folddef_h,
  c_args: luajit_cflags,
  include_directories: include_directories('src'),
  dependencies: [libm, libdl],
)

luajit_inc = include_directories('src')

luajit_dep = declare_dependency(
  link_with: luajit_lib,
  include_directories: luajit_inc,
  compile_args: ['-DLUAJIT_ENABLE_LUA52COMPAT'],
)
