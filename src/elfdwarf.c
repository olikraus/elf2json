/*

  elfdwarf.c


  Copyright (C) 2024  olikraus@gmail.com

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  Show dwarf information of a elf file

  Notes:

  A variable definition (the unit where it occupies memory) contains the 
    DW_AT_location
  attribute.
  Example:
    DW_TAG_variable (52): name 'file_2_b', code 2, attr cnt 7
    - 01/07 DW_AT_name (3)/DW_FORM_strp (14) 'file_2_b'
    - 02/07 DW_AT_decl_file (58)/DW_FORM_data1 (11) '1/0x1'
    - 03/07 DW_AT_decl_line (59)/DW_FORM_data1 (11) '4/0x4'
    - 04/07 DW_AT_decl_column (57)/DW_FORM_data1 (11) '5/0x5'
    - 05/07 DW_AT_type (73)/DW_FORM_ref4 (19) '210/0xd2'
    - 06/07 DW_AT_external (63)/DW_FORM_flag_present (25) ''
    - 07/07 DW_AT_location (2)/DW_FORM_exprloc (24) ''
  If both a definition and declartion exists (declaration first), then there are two entries:
    DW_TAG_variable (52): name 'file_2_b', code 2, attr cnt 7
    - 01/07 DW_AT_name (3)/DW_FORM_strp (14) 'file_2_b'
    - 02/07 DW_AT_decl_file (58)/DW_FORM_data1 (11) '1/0x1'
    - 03/07 DW_AT_decl_line (59)/DW_FORM_data1 (11) '3/0x3'
    - 04/07 DW_AT_decl_column (57)/DW_FORM_data1 (11) '12/0xc'
    - 05/07 DW_AT_type (73)/DW_FORM_ref4 (19) '200/0xc8'
    - 06/07 DW_AT_external (63)/DW_FORM_flag_present (25) ''
    - 07/07 DW_AT_declaration (60)/DW_FORM_flag_present (25) ''
  DW_TAG_variable (52): name 'n.a.', code 4, attr cnt 4
    - 01/04 DW_AT_specification (71)/DW_FORM_ref4 (19) '188/0xbc'
    - 02/04 DW_AT_decl_line (59)/DW_FORM_data1 (11) '4/0x4'
    - 03/04 DW_AT_decl_column (57)/DW_FORM_data1 (11) '5/0x5'
    - 04/04 DW_AT_location (2)/DW_FORM_exprloc (24) ''
  The DW_AT_specification refers to the previous declaration
  If the definition is first and the declaration follows later (doesn't make much sense),
  then again there is only one entry.

  probably the same topic with DW_TAG_subprogram
  case without prototype:
< 1><0x00000052>    DW_TAG_subprogram
                      DW_AT_external              yes(1)
                      DW_AT_name                  file_2_fn
                      DW_AT_decl_file             0x00000001 /home/kraus/git/elf2json/test/file_2.c
                      DW_AT_decl_line             0x00000006
                      DW_AT_decl_column           0x00000007
                      DW_AT_prototyped            yes(1)
                      DW_AT_low_pc                0x00001157
                      DW_AT_high_pc               <offset-from-lowpc> 21 <highpc: 0x0000116c>
                      DW_AT_frame_base            len 0x0001: 0x9c: 
                          DW_OP_call_frame_cfa
                      DW_AT_call_all_calls        yes(1)  
  The prototype looks like this:
  < 1><0x00000057>    DW_TAG_subprogram
                      DW_AT_external              yes(1)
                      DW_AT_name                  file_2_fn
                      DW_AT_decl_file             0x00000001 /home/kraus/git/elf2json/test/file_1.c
                      DW_AT_decl_line             0x00000005
                      DW_AT_decl_column           0x00000006
                      DW_AT_prototyped            yes(1)
                      DW_AT_declaration           yes(1)
  In general it seems, that if both exists, then the real code code contains DW_AT_low_pc/DW_AT_high_pc.
  According to the DWARF spec:
    "The module entry may have either a DW_AT_low_pc and DW_AT_high_pc pair
    of attributes or a DW_AT_ranges attribute"
  The subprogram may also have DW_AT_specification
  

  Get DIE by offset:
    int dwarf_offdie(Dwarf_Debug dbg, Dwarf_Off offset, Dwarf_Die *ret_die, Dwarf_Error *err);
	int dwarf_offdie_b(Dwarf_Debug dbg,	Dwarf_Off offset, Dwarf_Bool is_info, Dwarf_Die *ret_die, Dwarf_Error *err);

  Get offset of the DIE
  int dwarf_dieoffset(Dwarf_Die die, Dwarf_Off *ret_offset, Dwarf_Error *err);
*/

#include <stdio.h>
#include <assert.h>
//#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

//#include <libelf.h>
//#include <gelf.h>


#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

struct _dwarf_translate_struct
{
  size_t n;                     // number
  char *m;        // macro
};
typedef struct _dwarf_translate_struct dwarf_translate_struct;



#ifdef DW_LIBDWARF_VERSION_MINOR
/* libdwarf */
#define MY_DWARF_FINISH(dbg, err) dwarf_finish(dbg)
 /*
msys libdwarf:
  DW_API int dwarf_init_b(int dw_fd,
    unsigned int      dw_groupnumber,
    Dwarf_Handler     dw_errhand,
    Dwarf_Ptr         dw_errarg,
    Dwarf_Debug*      dw_dbg,
    Dwarf_Error*      dw_error);
  */
#define MY_DWARF_INIT(fd, dbg, err) \
	dwarf_init_b(fd, /*groupnumber (0=debug_info)*/ 0, /* error handler */ NULL, /* arg for error handler*/ NULL, dbg, err)
#else
/* ubuntu */
#define MY_DWARF_FINISH(dbg, err) dwarf_finish(dbg, err)
#ifdef COMMENT
/* for unix there is an additional "access" argument */
int dwarf_init_b(int    /*fd*/,
    Dwarf_Unsigned    /*access*/,
    unsigned int      /*groupnumber*/,
    Dwarf_Handler     /*errhand*/,
    Dwarf_Ptr         /*errarg*/,
    Dwarf_Debug*      /*dbg*/,
    Dwarf_Error*      /*error*/);
#endif
#define MY_DWARF_INIT(fd, dbg, err) \
	dwarf_init_b(fd, /*access (0=read)*/ 0, /*groupnumber (0=debug_info)*/ 0, /* error handler */ NULL, /* arg for error handler*/ NULL, dbg, err)
#endif

#define DT(c)  { c, #c}
#define DTNONE() { 0, NULL }


dwarf_translate_struct dwarf_tags[] = 
{
DT(DW_TAG_array_type),
DT(DW_TAG_class_type),
DT(DW_TAG_entry_point),
DT(DW_TAG_enumeration_type),
DT(DW_TAG_formal_parameter),
DT(DW_TAG_imported_declaration),
DT(DW_TAG_label),
DT(DW_TAG_lexical_block),
DT(DW_TAG_member),
DT(DW_TAG_pointer_type),
DT(DW_TAG_reference_type),
DT(DW_TAG_compile_unit),
DT(DW_TAG_string_type),
DT(DW_TAG_structure_type),
DT(DW_TAG_subroutine_type),
DT(DW_TAG_typedef),
DT(DW_TAG_union_type),
DT(DW_TAG_unspecified_parameters),
DT(DW_TAG_variant),
DT(DW_TAG_common_block),
DT(DW_TAG_common_inclusion),
DT(DW_TAG_inheritance),
DT(DW_TAG_inlined_subroutine),
DT(DW_TAG_module),
DT(DW_TAG_ptr_to_member_type),
DT(DW_TAG_set_type),
DT(DW_TAG_subrange_type),
DT(DW_TAG_with_stmt),
DT(DW_TAG_access_declaration),
DT(DW_TAG_base_type),
DT(DW_TAG_catch_block),
DT(DW_TAG_const_type),
DT(DW_TAG_constant),
DT(DW_TAG_enumerator),
DT(DW_TAG_file_type),
DT(DW_TAG_friend),
DT(DW_TAG_namelist),
DT(DW_TAG_namelist_item), 
DT(DW_TAG_packed_type),
DT(DW_TAG_subprogram),
DT(DW_TAG_template_type_parameter), 
DT(DW_TAG_template_value_parameter), 
DT(DW_TAG_thrown_type),
DT(DW_TAG_try_block),
DT(DW_TAG_variant_part),
DT(DW_TAG_variable),
DT(DW_TAG_volatile_type),
DT(DW_TAG_dwarf_procedure),  
DT(DW_TAG_restrict_type),  
DT(DW_TAG_interface_type),  
DT(DW_TAG_namespace),  
DT(DW_TAG_imported_module),  
DT(DW_TAG_unspecified_type),  
DT(DW_TAG_partial_unit),  
DT(DW_TAG_imported_unit),  
DT(DW_TAG_mutable_type), 
DT(DW_TAG_condition),  
DT(DW_TAG_shared_type),  
DT(DW_TAG_type_unit),  
DT(DW_TAG_rvalue_reference_type),  
DT(DW_TAG_template_alias),  
DT(DW_TAG_coarray_type),  
DT(DW_TAG_generic_subrange),  
DT(DW_TAG_dynamic_type),  
DT(DW_TAG_atomic_type),  
DT(DW_TAG_call_site),  
DT(DW_TAG_call_site_parameter),  
DT(DW_TAG_skeleton_unit),  
DT(DW_TAG_immutable_type),  
DT(DW_TAG_lo_user),
DT(DW_TAG_MIPS_loop),
DT(DW_TAG_HP_array_descriptor), 
DT(DW_TAG_format_label), 
DT(DW_TAG_function_template), 
DT(DW_TAG_class_template), 
DT(DW_TAG_GNU_BINCL), 
DT(DW_TAG_GNU_EINCL), 
DT(DW_TAG_GNU_template_template_parameter), 
DT(DW_TAG_GNU_template_template_param), 
DT(DW_TAG_GNU_template_parameter_pack), 
DT(DW_TAG_GNU_formal_parameter_pack), 
DT(DW_TAG_GNU_call_site), 
DT(DW_TAG_GNU_call_site_parameter), 
DT(DW_TAG_ALTIUM_circ_type), 
DT(DW_TAG_ALTIUM_mwa_circ_type), 
DT(DW_TAG_ALTIUM_rev_carry_type), 
DT(DW_TAG_ALTIUM_rom), 
DT(DW_TAG_upc_shared_type), 
DT(DW_TAG_upc_strict_type), 
DT(DW_TAG_upc_relaxed_type), 
DT(DW_TAG_SUN_function_template), 
DT(DW_TAG_SUN_class_template), 
DT(DW_TAG_SUN_struct_template), 
DT(DW_TAG_SUN_union_template), 
DT(DW_TAG_SUN_indirect_inheritance), 
DT(DW_TAG_SUN_codeflags), 
DT(DW_TAG_SUN_memop_info), 
DT(DW_TAG_SUN_omp_child_func), 
DT(DW_TAG_SUN_rtti_descriptor), 
DT(DW_TAG_SUN_dtor_info), 
DT(DW_TAG_SUN_dtor), 
DT(DW_TAG_SUN_f90_interface), 
DT(DW_TAG_SUN_fortran_vax_structure), 
DT(DW_TAG_SUN_hi), 
DT(DW_TAG_ghs_namespace),
DT(DW_TAG_ghs_using_namespace),
DT(DW_TAG_ghs_using_declaration),
DT(DW_TAG_ghs_template_templ_param),
DT(DW_TAG_PGI_kanji_type), 
DT(DW_TAG_PGI_interface_block), 
DT(DW_TAG_BORLAND_property),
DT(DW_TAG_BORLAND_Delphi_string),
DT(DW_TAG_BORLAND_Delphi_dynamic_array),
DT(DW_TAG_BORLAND_Delphi_set),
DT(DW_TAG_BORLAND_Delphi_variant),
DT(DW_TAG_hi_user),
DTNONE()
};

dwarf_translate_struct dwarf_forms[] = 
{
DT(DW_FORM_addr),
DT(DW_FORM_block2),
DT(DW_FORM_block4),
DT(DW_FORM_data2),
DT(DW_FORM_data4),
DT(DW_FORM_data8),
DT(DW_FORM_string),
DT(DW_FORM_block),
DT(DW_FORM_block1),
DT(DW_FORM_data1),
DT(DW_FORM_flag),
DT(DW_FORM_sdata),
DT(DW_FORM_strp),
DT(DW_FORM_udata),
DT(DW_FORM_ref_addr),
DT(DW_FORM_ref1),
DT(DW_FORM_ref2),
DT(DW_FORM_ref4),
DT(DW_FORM_ref8),
DT(DW_FORM_ref_udata),
DT(DW_FORM_indirect),
DT(DW_FORM_sec_offset), 
DT(DW_FORM_exprloc), 
DT(DW_FORM_flag_present), 
DT(DW_FORM_strx), 
DT(DW_FORM_addrx), 
DT(DW_FORM_ref_sup4), 
DT(DW_FORM_strp_sup), 
DT(DW_FORM_data16), 
DT(DW_FORM_line_strp), 
DT(DW_FORM_ref_sig8), 
DT(DW_FORM_implicit_const), 
DT(DW_FORM_loclistx), 
DT(DW_FORM_rnglistx), 
DT(DW_FORM_ref_sup8), 
DT(DW_FORM_strx1), 
DT(DW_FORM_strx2), 
DT(DW_FORM_strx3), 
DT(DW_FORM_strx4), 
DT(DW_FORM_addrx1), 
DT(DW_FORM_addrx2), 
DT(DW_FORM_addrx3), 
DT(DW_FORM_addrx4), 
DT(DW_FORM_GNU_addr_index), 
DT(DW_FORM_GNU_str_index),
DT(DW_FORM_GNU_ref_alt), 
DT(DW_FORM_GNU_strp_alt),
DTNONE()
};


dwarf_translate_struct dwarf_attributes[] = 
{
DT(DW_AT_sibling),
DT(DW_AT_location),
DT(DW_AT_name),
DT(DW_AT_ordering),
DT(DW_AT_subscr_data),
DT(DW_AT_byte_size),
DT(DW_AT_bit_offset),
DT(DW_AT_bit_size),
DT(DW_AT_element_list),
DT(DW_AT_stmt_list),
DT(DW_AT_low_pc),
DT(DW_AT_high_pc),
DT(DW_AT_language),
DT(DW_AT_member),
DT(DW_AT_discr),
DT(DW_AT_discr_value),
DT(DW_AT_visibility),
DT(DW_AT_import),
DT(DW_AT_string_length),
DT(DW_AT_common_reference),
DT(DW_AT_comp_dir),
DT(DW_AT_const_value),
DT(DW_AT_containing_type),
DT(DW_AT_default_value),
DT(DW_AT_inline),
DT(DW_AT_is_optional),
DT(DW_AT_lower_bound),
DT(DW_AT_producer),
DT(DW_AT_prototyped),
DT(DW_AT_return_addr),
DT(DW_AT_start_scope),
DT(DW_AT_bit_stride), 
DT(DW_AT_stride_size), 
DT(DW_AT_upper_bound),
DT(DW_AT_abstract_origin),
DT(DW_AT_accessibility),
DT(DW_AT_address_class),
DT(DW_AT_artificial),
DT(DW_AT_base_types),
DT(DW_AT_calling_convention),
DT(DW_AT_count),
DT(DW_AT_data_member_location),
DT(DW_AT_decl_column),
DT(DW_AT_decl_file),
DT(DW_AT_decl_line),
DT(DW_AT_declaration),
DT(DW_AT_discr_list), 
DT(DW_AT_encoding),
DT(DW_AT_external),
DT(DW_AT_frame_base),
DT(DW_AT_friend),
DT(DW_AT_identifier_case),
DT(DW_AT_macro_info), 
DT(DW_AT_namelist_item),
DT(DW_AT_priority),
DT(DW_AT_segment),
DT(DW_AT_specification),
DT(DW_AT_static_link),
DT(DW_AT_type),
DT(DW_AT_use_location),
DT(DW_AT_variable_parameter),
DT(DW_AT_virtuality),
DT(DW_AT_vtable_elem_location),
DT(DW_AT_allocated), 
DT(DW_AT_associated), 
DT(DW_AT_data_location), 
DT(DW_AT_byte_stride), 
DT(DW_AT_stride), 
DT(DW_AT_entry_pc), 
DT(DW_AT_use_UTF8), 
DT(DW_AT_extension), 
DT(DW_AT_ranges), 
DT(DW_AT_trampoline), 
DT(DW_AT_call_column), 
DT(DW_AT_call_file), 
DT(DW_AT_call_line), 
DT(DW_AT_description), 
DT(DW_AT_binary_scale), 
DT(DW_AT_decimal_scale), 
DT(DW_AT_small), 
DT(DW_AT_decimal_sign), 
DT(DW_AT_digit_count), 
DT(DW_AT_picture_string), 
DT(DW_AT_mutable), 
DT(DW_AT_threads_scaled), 
DT(DW_AT_explicit), 
DT(DW_AT_object_pointer), 
DT(DW_AT_endianity), 
DT(DW_AT_elemental), 
DT(DW_AT_pure), 
DT(DW_AT_recursive), 
DT(DW_AT_signature), 
DT(DW_AT_main_subprogram), 
DT(DW_AT_data_bit_offset), 
DT(DW_AT_const_expr), 
DT(DW_AT_enum_class), 
DT(DW_AT_linkage_name), 
DT(DW_AT_string_length_bit_size), 
DT(DW_AT_string_length_byte_size), 
DT(DW_AT_rank), 
DT(DW_AT_str_offsets_base), 
DT(DW_AT_addr_base), 
DT(DW_AT_rnglists_base), 
DT(DW_AT_dwo_id), 
DT(DW_AT_dwo_name), 
DT(DW_AT_reference), 
DT(DW_AT_rvalue_reference), 
DT(DW_AT_macros), 
DT(DW_AT_call_all_calls), 
DT(DW_AT_call_all_source_calls), 
DT(DW_AT_call_all_tail_calls), 
DT(DW_AT_call_return_pc), 
DT(DW_AT_call_value), 
DT(DW_AT_call_origin), 
DT(DW_AT_call_parameter), 
DT(DW_AT_call_pc), 
DT(DW_AT_call_tail_call), 
DT(DW_AT_call_target), 
DT(DW_AT_call_target_clobbered), 
DT(DW_AT_call_data_location), 
DT(DW_AT_call_data_value), 
DT(DW_AT_noreturn), 
DT(DW_AT_alignment), 
DT(DW_AT_export_symbols), 
DT(DW_AT_deleted), 
DT(DW_AT_defaulted), 
DT(DW_AT_loclists_base), 
DT(DW_AT_ghs_namespace_alias),
DT(DW_AT_ghs_using_namespace),
DT(DW_AT_ghs_using_declaration),
DT(DW_AT_HP_block_index),  
DT(DW_AT_lo_user),
DT(DW_AT_MIPS_fde), 
DT(DW_AT_MIPS_loop_begin), 
DT(DW_AT_MIPS_tail_loop_begin), 
DT(DW_AT_MIPS_epilog_begin), 
DT(DW_AT_MIPS_loop_unroll_factor), 
DT(DW_AT_MIPS_software_pipeline_depth), 
DT(DW_AT_MIPS_linkage_name), 
DT(DW_AT_MIPS_stride), 
DT(DW_AT_MIPS_abstract_name), 
DT(DW_AT_MIPS_clone_origin), 
DT(DW_AT_MIPS_has_inlines), 
DT(DW_AT_MIPS_stride_byte), 
DT(DW_AT_MIPS_stride_elem), 
DT(DW_AT_MIPS_ptr_dopetype), 
DT(DW_AT_MIPS_allocatable_dopetype), 
DT(DW_AT_MIPS_assumed_shape_dopetype), 
DT(DW_AT_MIPS_assumed_size), 
DT(DW_AT_HP_unmodifiable), 
DT(DW_AT_HP_prologue), 
DT(DW_AT_HP_epilogue), 
DT(DW_AT_HP_actuals_stmt_list), 
DT(DW_AT_HP_proc_per_section), 
DT(DW_AT_HP_raw_data_ptr), 
DT(DW_AT_HP_pass_by_reference), 
DT(DW_AT_HP_opt_level), 
DT(DW_AT_HP_prof_version_id), 
DT(DW_AT_HP_opt_flags), 
DT(DW_AT_HP_cold_region_low_pc), 
DT(DW_AT_HP_cold_region_high_pc), 
DT(DW_AT_HP_all_variables_modifiable), 
DT(DW_AT_HP_linkage_name), 
DT(DW_AT_HP_prof_flags), 
DT(DW_AT_HP_unit_name),
DT(DW_AT_HP_unit_size),
DT(DW_AT_HP_widened_byte_size),
DT(DW_AT_HP_definition_points),
DT(DW_AT_HP_default_location),
DT(DW_AT_HP_is_result_param),
DT(DW_AT_CPQ_discontig_ranges), 
DT(DW_AT_CPQ_semantic_events), 
DT(DW_AT_CPQ_split_lifetimes_var), 
DT(DW_AT_CPQ_split_lifetimes_rtn), 
DT(DW_AT_CPQ_prologue_length), 
DT(DW_AT_ghs_mangled),  
DT(DW_AT_ghs_rsm),
DT(DW_AT_ghs_frsm),
DT(DW_AT_ghs_frames),
DT(DW_AT_ghs_rso),
DT(DW_AT_ghs_subcpu),
DT(DW_AT_ghs_lbrace_line),
DT(DW_AT_INTEL_other_endian), 
DT(DW_AT_sf_names), 
DT(DW_AT_src_info), 
DT(DW_AT_mac_info), 
DT(DW_AT_src_coords), 
DT(DW_AT_body_begin), 
DT(DW_AT_body_end), 
DT(DW_AT_GNU_vector), 
DT(DW_AT_GNU_guarded_by), 
DT(DW_AT_GNU_pt_guarded_by), 
DT(DW_AT_GNU_guarded), 
DT(DW_AT_GNU_pt_guarded), 
DT(DW_AT_GNU_locks_excluded), 
DT(DW_AT_GNU_exclusive_locks_required), 
DT(DW_AT_GNU_shared_locks_required), 
DT(DW_AT_GNU_odr_signature), 
DT(DW_AT_GNU_template_name), 
DT(DW_AT_GNU_call_site_value), 
DT(DW_AT_GNU_call_site_data_value), 
DT(DW_AT_GNU_call_site_target), 
DT(DW_AT_GNU_call_site_target_clobbered), 
DT(DW_AT_GNU_tail_call), 
DT(DW_AT_GNU_all_tail_call_sites), 
DT(DW_AT_GNU_all_call_sites), 
DT(DW_AT_GNU_all_source_call_sites), 
DT(DW_AT_GNU_macros), 
DT(DW_AT_GNU_deleted), 
DT(DW_AT_GNU_dwo_name), 
DT(DW_AT_GNU_dwo_id), 
DT(DW_AT_GNU_ranges_base), 
DT(DW_AT_GNU_addr_base), 
DT(DW_AT_GNU_pubnames), 
DT(DW_AT_GNU_pubtypes), 
DT(DW_AT_GNU_discriminator), 
DT(DW_AT_GNU_locviews), 
DT(DW_AT_GNU_entry_view), 
DT(DW_AT_GNU_bias),
DT(DW_AT_SUN_template), 
DT(DW_AT_VMS_rtnbeg_pd_address), 
DT(DW_AT_SUN_alignment), 
DT(DW_AT_SUN_vtable), 
DT(DW_AT_SUN_count_guarantee), 
DT(DW_AT_SUN_command_line), 
DT(DW_AT_SUN_vbase), 
DT(DW_AT_SUN_compile_options), 
DT(DW_AT_SUN_language), 
DT(DW_AT_SUN_browser_file), 
DT(DW_AT_SUN_vtable_abi), 
DT(DW_AT_SUN_func_offsets), 
DT(DW_AT_SUN_cf_kind), 
DT(DW_AT_SUN_vtable_index), 
DT(DW_AT_SUN_omp_tpriv_addr), 
DT(DW_AT_SUN_omp_child_func), 
DT(DW_AT_SUN_func_offset), 
DT(DW_AT_SUN_memop_type_ref), 
DT(DW_AT_SUN_profile_id), 
DT(DW_AT_SUN_memop_signature), 
DT(DW_AT_SUN_obj_dir), 
DT(DW_AT_SUN_obj_file), 
DT(DW_AT_SUN_original_name), 
DT(DW_AT_SUN_hwcprof_signature), 
DT(DW_AT_SUN_amd64_parmdump), 
DT(DW_AT_SUN_part_link_name), 
DT(DW_AT_SUN_link_name), 
DT(DW_AT_SUN_pass_with_const), 
DT(DW_AT_SUN_return_with_const), 
DT(DW_AT_SUN_import_by_name), 
DT(DW_AT_SUN_f90_pointer), 
DT(DW_AT_SUN_pass_by_ref), 
DT(DW_AT_SUN_f90_allocatable), 
DT(DW_AT_SUN_f90_assumed_shape_array), 
DT(DW_AT_SUN_c_vla), 
DT(DW_AT_SUN_return_value_ptr), 
DT(DW_AT_SUN_dtor_start), 
DT(DW_AT_SUN_dtor_length), 
DT(DW_AT_SUN_dtor_state_initial), 
DT(DW_AT_SUN_dtor_state_final), 
DT(DW_AT_SUN_dtor_state_deltas), 
DT(DW_AT_SUN_import_by_lname), 
DT(DW_AT_SUN_f90_use_only), 
DT(DW_AT_SUN_namelist_spec), 
DT(DW_AT_SUN_is_omp_child_func), 
DT(DW_AT_SUN_fortran_main_alias), 
DT(DW_AT_SUN_fortran_based), 
DT(DW_AT_ALTIUM_loclist),          
DT(DW_AT_use_GNAT_descriptive_type),
DT(DW_AT_GNAT_descriptive_type),
DT(DW_AT_GNU_numerator), 
DT(DW_AT_GNU_denominator), 
DT(DW_AT_GNU_bias), 
DT(DW_AT_go_kind),
DT(DW_AT_go_key),
DT(DW_AT_go_elem),
DT(DW_AT_go_embedded_field),
DT(DW_AT_go_runtime_type),
DT(DW_AT_upc_threads_scaled), 
DT(DW_AT_IBM_wsa_addr),
DT(DW_AT_IBM_home_location),
DT(DW_AT_IBM_alt_srcview),
DT(DW_AT_PGI_lbase),
DT(DW_AT_PGI_soffset),
DT(DW_AT_PGI_lstride),
DT(DW_AT_BORLAND_property_read),
DT(DW_AT_BORLAND_property_write),
DT(DW_AT_BORLAND_property_implements),
DT(DW_AT_BORLAND_property_index),
DT(DW_AT_BORLAND_property_default),
DT(DW_AT_BORLAND_Delphi_unit),
DT(DW_AT_BORLAND_Delphi_class),
DT(DW_AT_BORLAND_Delphi_record),
DT(DW_AT_BORLAND_Delphi_metaclass),
DT(DW_AT_BORLAND_Delphi_constructor),
DT(DW_AT_BORLAND_Delphi_destructor),
DT(DW_AT_BORLAND_Delphi_anonymous_method),
DT(DW_AT_BORLAND_Delphi_interface),
DT(DW_AT_BORLAND_Delphi_ABI),
DT(DW_AT_BORLAND_Delphi_frameptr),
DT(DW_AT_BORLAND_closure),
DT(DW_AT_LLVM_include_path),
DT(DW_AT_LLVM_config_macros),
DT(DW_AT_LLVM_sysroot),
DT(DW_AT_LLVM_tag_offset),
DT(DW_AT_LLVM_apinotes),
DT(DW_AT_APPLE_optimized),
DT(DW_AT_APPLE_flags),
DT(DW_AT_APPLE_isa),
DT(DW_AT_APPLE_block),
DT(DW_AT_APPLE_major_runtime_vers),
DT(DW_AT_APPLE_runtime_class),
DT(DW_AT_APPLE_omit_frame_ptr),
DT(DW_AT_APPLE_property_name),
DT(DW_AT_APPLE_property_getter),
DT(DW_AT_APPLE_property_setter),
DT(DW_AT_APPLE_property_attribute),
DT(DW_AT_APPLE_objc_complete_type),
DT(DW_AT_APPLE_property),
DT(DW_AT_APPLE_objc_direct),
DT(DW_AT_APPLE_sdk),
DT(DW_AT_hi_user),
DTNONE()
};

const char *dwarf_id2str(dwarf_translate_struct *dt, size_t id)
{
  while( dt->m != NULL )
  {
    if ( dt->n == id )
      return dt->m;
    dt++;
  }
  return "";
}


void print_indent(int indent)
{
  while( indent > 0 )
  {
    printf("  ");
    indent--;
  }
}


/*
  get unsigned integer value of an attribute.
  do this by trying several form functions
  partly taken over from dwarfdump
*/
int dwarf_get_unsigned_attribute_value(Dwarf_Attribute attribute, Dwarf_Unsigned *uval_out, Dwarf_Error *err)
{
    Dwarf_Unsigned uval = 0;
    int ret = dwarf_formudata(attribute, &uval, err);
    if (ret != DW_DLV_OK) 
    {
        Dwarf_Signed sval = 0;
        ret = dwarf_formsdata(attribute, &sval, err);
        if (ret != DW_DLV_OK) 
        {
            ret = dwarf_global_formref(attribute,&uval,err);
            if (ret != DW_DLV_OK) 
            {
                return ret;
            }
            else
            {
              *uval_out = uval;
            }
        } 
        else 
        {
            *uval_out = (Dwarf_Unsigned)sval;
        }
    } 
    else 
    {
        *uval_out = uval;
    }
    return DW_DLV_OK;
}


int show_die(Dwarf_Debug dbg, int indent, Dwarf_Die die)
{
  Dwarf_Error err;
  Dwarf_Half tag;
  char *name;
  int abbrev_code;
  
  Dwarf_Signed attr_count, i;
  Dwarf_Attribute *attr_list;  
  Dwarf_Off offset;

  if ( dwarf_tag(die, &tag, &err)  != DW_DLV_OK)
    return fprintf(stderr, "dwarf_tag: %s\n", dwarf_errmsg(err) ), 0;
    
  if ( dwarf_diename(die, &name, &err)  != DW_DLV_OK)
    name = "n.a.";
  
  abbrev_code = dwarf_die_abbrev_code(die);
  attr_count = 0;
  
  if ( dwarf_attrlist(die, &attr_list, &attr_count, &err) != DW_DLV_OK)
    attr_count = 0;
  
  if ( dwarf_dieoffset(die, &offset, &err) != DW_DLV_OK)
    offset = 0;
  
  print_indent(indent);
  printf("<0x%08llx> %s (%u): name '%s', code %d, attr cnt %lld\n", (long long unsigned)offset, dwarf_id2str(dwarf_tags, tag), (unsigned)tag, name, abbrev_code, (long long int)attr_count);
  
  for( i = 0; i < attr_count; i++ )
  {
    Dwarf_Half attr_num;
    Dwarf_Half form;
    char *str;  
    Dwarf_Unsigned uval;
    Dwarf_Addr address;
    char buf[64];
    
    if ( dwarf_whatattr(attr_list[i], &attr_num, &err) != DW_DLV_OK)
      return fprintf(stderr, "dwarf_whatattr: %s\n", dwarf_errmsg(err) ), 0;
      
    if ( dwarf_whatform(attr_list[i], &form, &err) != DW_DLV_OK)
      return fprintf(stderr, "dwarf_whatform: %s\n", dwarf_errmsg(err) ), 0;
    
    /* try to convert the attribute value to some readable value. result is stored in "str" */
    /*
      todo: handle source file names and DW_FORM_implicit_const
      https://github.com/avast/libdwarf/blob/7d4f63f44d44386744628ee1bca84afc43a56583/libdwarf/dwarfdump/print_die.c#L3880
    
      instead of using udata, we might use this approach: 
      https://github.com/avast/libdwarf/blob/7d4f63f44d44386744628ee1bca84afc43a56583/libdwarf/dwarfdump/print_die.c#L1296
    */
    if ( dwarf_formstring(attr_list[i], &str, &err) != DW_DLV_OK )
    {
      if ( dwarf_get_unsigned_attribute_value(attr_list[i], &uval, &err) == DW_DLV_OK )
      {
        sprintf(buf, "%llu/0x%llx", (long long unsigned)uval, (long long unsigned)uval);
        str = buf;
      }
      else
      {
        if ( dwarf_formaddr(attr_list[i], &address, &err) == DW_DLV_OK )
        {
          sprintf(buf, "0x%08llx", (long long unsigned)address);
          str = buf;
        }
        else
        {
          str = "";
        }
      }
    }
    print_indent(indent);
    printf("  - %02lld/%02lld %s (%d)/%s (%d) '%s'\n", 
      (long long int)i+(long long int)1, (long long int)attr_count, 
      dwarf_id2str(dwarf_attributes, attr_num), attr_num, 
      dwarf_id2str(dwarf_forms, form), form,
      str
    );
    if ( attr_num == DW_AT_specification )
    {
        Dwarf_Die spec_die;
        if ( dwarf_offdie_b(dbg, (Dwarf_Off)uval, /* is_info= */ 1, &spec_die, &err) == DW_DLV_OK )
          show_die(dbg, indent+2, spec_die);
    }
  }
  
  return 1;
}

int dfs_die(Dwarf_Debug dbg, int indent, Dwarf_Die die)
{
  Dwarf_Error err;
  Dwarf_Die next_die;
  Dwarf_Die child_die;
  int r;
  for(;;)
  {
    show_die(dbg, indent, die);
    
   if (dwarf_child(die, &child_die, &err) == DW_DLV_OK)
   {
     if ( dfs_die(dbg, indent+1, child_die) == 0 )
       return 0;
   }
   
    next_die = NULL;
    r = dwarf_siblingof_b(dbg, die, /* is_info= */ 1, &next_die, &err);
    if ( r == DW_DLV_ERROR )
      return fprintf(stderr, "dwarf_siblingof: %s\n", dwarf_errmsg(err) ), 0;
    if ( r == DW_DLV_NO_ENTRY )
      break;
    if ( next_die == NULL )
      break;
    die = next_die;
  }
  return 1;
}

int show_dwarf(int fd)
{
  Dwarf_Error err;
  Dwarf_Debug dbg;
  Dwarf_Die die;
  Dwarf_Signed srcfile_cnt = 0;
  Dwarf_Unsigned next_cu_header_offset;
  char **srcfile_list = 0;
  int ret;

  //if (dwarf_elf_init(elf, DW_DLC_READ, NULL, NULL, &dbg, &err) != DW_DLV_OK)
  //  return fprintf(stderr, "dwarf_init: %s\n", dwarf_errmsg(err) ), 0;
  /* https://sources.debian.org/data/main/d/dwarfutils/20210528-1/libdwarf/libdwarf2.1.pdf */
  
  
  if ( MY_DWARF_INIT(fd, &dbg, &err) != DW_DLV_OK)
  {
    return fprintf(stderr, "dwarf_init_b: %s\n", dwarf_errmsg(err) ), 0;
  }
  
  for(;;)
  {


    /*
      int dwarf_next_cu_header_d(Dwarf_Debug dbg, 
          Dwarf_Bool      is_info,
          Dwarf_Unsigned* cu_header_length,
          Dwarf_Half*     version_stamp,
          Dwarf_Off*      abbrev_offset,
          Dwarf_Half*     address_size,
          Dwarf_Half*     length_size,
          Dwarf_Half*     extension_size,
          Dwarf_Sig8*     type signature,
          Dwarf_Unsigned* typeoffset,
          Dwarf_Unsigned* next_cu_header_offset,
          Dwarf_Half    * header_cu_type,
          Dwarf_Error*    error);    
    */
    /* get the first or next unit */
    ret = dwarf_next_cu_header_d(dbg, /*is_info*/1, /*cu_header_length*/ NULL, /* version_stamp*/ NULL, 
      /*abbrev_offset*/ NULL, /*address_size*/ NULL, /*length_size*/NULL, /*extension_size*/NULL,/*signature*/NULL,
      /*typeoffset*/ NULL, /*next_cu_header_offset*/&next_cu_header_offset, /*header_cu_type*/NULL, &err);
    //ret = dwarf_next_cu_header(dbg, NULL, NULL, NULL, NULL, NULL, &err);
	// todo: use dwarf_next_cu_header_d or dwarf_next_cu_header_e
    if ( ret == DW_DLV_ERROR)
      return fprintf(stderr, "dwarf_next_cu_header: %s\n", dwarf_errmsg(err) ), 0;
    if ( ret == DW_DLV_NO_ENTRY )
    {
      break;
    }
    
    /* get first DIE */
    if (dwarf_siblingof_b(dbg, NULL, /* is_info= */ 1, &die, &err) != DW_DLV_OK)
      return fprintf(stderr, "dwarf_siblingof: %s\n", dwarf_errmsg(err) ), 0;

    /* it is assumed, that the DIE is a compilation unit. for such a CU DIE, get the source files (includes) */
    /* such files are referenced by DW_AT_decl_file */
    if ( dwarf_srcfiles(die, &srcfile_list, &srcfile_cnt, &err)  != DW_DLV_OK) 
    {
      srcfile_list = 0;
      srcfile_cnt = 0;
    }
    else
    {
      Dwarf_Signed i;
      for( i = 0; i < srcfile_cnt; i++ )
        printf("File %lli: %s\n", (long long int)i, srcfile_list[i]);
    }
    
    dfs_die(dbg, 0, die);
  }
  
  MY_DWARF_FINISH(dbg, &err);	
  return 1;
}

/*=========================================*/

const char *dwarf_get_die_name(Dwarf_Debug dbg, Dwarf_Die die)
{
  Dwarf_Error err;
  Dwarf_Attribute attribute;
  Dwarf_Off offset;
  Dwarf_Die spec_die;
  char *name = NULL;
  
  if ( dwarf_diename(die, &name, &err)  == DW_DLV_OK)
    return name;
  
  /* the name is not there, check, if there is a DW_AT_specification reference */
  if ( dwarf_attr(die, DW_AT_specification, &attribute, &err ) != DW_DLV_OK )
    return NULL;        // no reference to a different DIE, so we don't have a name
  
  /* we have a valid DW_AT_specification attribute, get the value for the same */
  if ( dwarf_global_formref(attribute, &offset, &err)  != DW_DLV_OK )
    return NULL;                // can't get the offset value

  /* with the offset, get the specification DIE */
  if ( dwarf_offdie_b(dbg, offset, /* is_info= */ 1, &spec_die, &err) != DW_DLV_OK )
    return NULL;        // can't get the referenced DIE
  
  return dwarf_get_die_name(dbg, spec_die);           // let's recur and return the name from the reference 
}

int dwarf_search_defs_dfs(Dwarf_Debug dbg, Dwarf_Die die, const char *cu_name)
{
  Dwarf_Error err;
  Dwarf_Die child_die;
  Dwarf_Die next_die;
  Dwarf_Half tag;
  Dwarf_Attribute attribute;
  int r;
  const char *fn_name;
  const char *var_name;

  /* loop over the DIE and it's siblings, recursive calls for any child DIE */
  for(;;)
  {
    /* look out for subprogram or variables TAGs */
    if ( dwarf_tag(die, &tag, &err)  != DW_DLV_OK)
      return MY_DWARF_FINISH(dbg, &err), 0;
    
    fn_name = NULL;
    var_name = NULL;
    
    if ( tag == DW_TAG_subprogram )
    {
      /* subprogram found, check whether this is a declaration or definition */
      /* it is a definition, if either DW_AT_low_pc or DW_AT_ranges attribute is there (see DWARF spec) */
      if ( dwarf_attr(die, DW_AT_low_pc, &attribute, &err ) == DW_DLV_OK )
        fn_name = dwarf_get_die_name(dbg, die);
      else if ( dwarf_attr(die, DW_AT_ranges, &attribute, &err ) == DW_DLV_OK )
        fn_name = dwarf_get_die_name(dbg, die);
      
      if ( fn_name != NULL )
        printf("CU: %s, Function: %s\n", cu_name, fn_name);
    }
    else if ( tag == DW_TAG_variable )
    {
      if ( dwarf_attr(die, DW_AT_location, &attribute, &err ) == DW_DLV_OK )
        var_name = dwarf_get_die_name(dbg, die);
      
      if ( var_name != NULL )
        printf("CU: %s, Variable: %s\n", cu_name, var_name);
    }

    if ( tag != DW_TAG_subprogram )     // skip functions, because we don't want local variables
    {
      /* if there is any child attched, recur to the child DIEs */
      if (dwarf_child(die, &child_die, &err) == DW_DLV_OK)               // is there any child?
         if ( dwarf_search_defs_dfs(dbg, child_die, cu_name) == 0 )                    // if so, do a recursive call
           return 0;
    }
   
    /* get the next DIE in the sequence */
    next_die = NULL;
    r = dwarf_siblingof_b(dbg, die, /* is_info= */ 1, &next_die, &err);
    if ( r == DW_DLV_ERROR )
      return 0;
    if ( r == DW_DLV_NO_ENTRY || next_die == NULL )                     // stop for loop if there are no more siblings
      break;
    die = next_die;
  }
  return 1;
  
}


/*
  show function and global variable definitions
*/
int show_definitions(int fd)
{
  Dwarf_Error err;
  Dwarf_Debug dbg;
  Dwarf_Die die;
  Dwarf_Half tag;
  Dwarf_Unsigned next_cu_header_offset;
  char *cu_name = NULL;
  int ret;
  
  /* https://sources.debian.org/data/main/d/dwarfutils/20210528-1/libdwarf/libdwarf2.1.pdf */
  if ( MY_DWARF_INIT(fd, &dbg, &err) != DW_DLV_OK)
  {
    //return fprintf(stderr, "dwarf_init_b: %s\n", dwarf_errmsg(err) ), 0;
    return 0;
  }

  /* loop over all compilation units */
  for(;;)
  {
     /*
      int dwarf_next_cu_header_d(Dwarf_Debug dbg, 
          Dwarf_Bool      is_info,
          Dwarf_Unsigned* cu_header_length,
          Dwarf_Half*     version_stamp,
          Dwarf_Off*      abbrev_offset,
          Dwarf_Half*     address_size,
          Dwarf_Half*     length_size,
          Dwarf_Half*     extension_size,
          Dwarf_Sig8*     type signature,
          Dwarf_Unsigned* typeoffset,
          Dwarf_Unsigned* next_cu_header_offset,
          Dwarf_Half    * header_cu_type,
          Dwarf_Error*    error);    
    */
    /* get the first or next unit */
    ret = dwarf_next_cu_header_d(dbg, /*is_info*/1, /*cu_header_length*/ NULL, /* version_stamp*/ NULL, 
      /*abbrev_offset*/ NULL, /*address_size*/ NULL, /*length_size*/NULL, /*extension_size*/NULL,/*signature*/NULL,
      /*typeoffset*/ NULL, /*next_cu_header_offset*/&next_cu_header_offset, /*header_cu_type*/NULL, &err);
   /* get the first or next unit */
    //ret = dwarf_next_cu_header(dbg, NULL, NULL, NULL, NULL, NULL, &err);
    if ( ret == DW_DLV_ERROR)
      return MY_DWARF_FINISH(dbg, &err), 0;
    if ( ret == DW_DLV_NO_ENTRY )
      break;
    
    /* get first DIE */
    if (dwarf_siblingof_b(dbg, NULL, /* is_info= */ 1, &die, &err) != DW_DLV_OK)
      return MY_DWARF_FINISH(dbg, &err), 0;

    /* get the tag of the DIE to validate whether this is really a compile unit */
    if ( dwarf_tag(die, &tag, &err)  != DW_DLV_OK)
      return MY_DWARF_FINISH(dbg, &err), 0;

    if ( tag == DW_TAG_compile_unit )
    {
      /* get the name of the compile unit */
      if ( dwarf_diename(die, &cu_name, &err)  == DW_DLV_OK)
      {
        if ( dwarf_search_defs_dfs(dbg, die, cu_name) == 0 )
          return MY_DWARF_FINISH(dbg, &err), 0;
      }
    }
  }
  
  MY_DWARF_FINISH(dbg, &err);	
  return 1;
}


/*=========================================*/


int main(int argc, char **argv)
{
  int fd = -1;
  char *elf_filename = NULL;
  if ( argc < 2 )
  {
    printf("%s <input.elf>\n", argv[0]);
    return 1;
  }

  elf_filename = argv[1];
  fd = open( elf_filename, O_RDONLY | O_BINARY , 0);
  if ( fd >= 0 )
  {
    if ( show_dwarf(fd) )
    {
      show_definitions(fd);
      close(fd);  
      return 0;
    }
    else
    {
      fprintf(stderr, "Conversion failed\n");
    }
    close(fd);
  }
  else
  {
    perror(elf_filename);
  }
  return 0;
}
