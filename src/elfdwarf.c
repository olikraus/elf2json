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
  
*/

#include <stdio.h>
//#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <libelf.h>
#include <gelf.h>


#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

struct _dwarf_translate_struct
{
  size_t n;                     // number
  char *m;        // macro
};
typedef struct _dwarf_translate_struct dwarf_translate_struct;


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


int show_die(int indent, Dwarf_Die die)
{
  Dwarf_Error err;
  Dwarf_Half tag;
  char *name;
  int abbrev_code;
  
  Dwarf_Signed attr_count, i;
  Dwarf_Attribute *attr_list;  

  if ( dwarf_tag(die, &tag, &err)  != DW_DLV_OK)
    return fprintf(stderr, "dwarf_tag: %s\n", dwarf_errmsg(err) ), 0;
    
  if ( dwarf_diename(die, &name, &err)  != DW_DLV_OK)
    name = "n.a.";
  
  abbrev_code = dwarf_die_abbrev_code(die);
  attr_count = 0;
  
  if ( dwarf_attrlist(die, &attr_list, &attr_count, &err) != DW_DLV_OK)
    attr_count = 0;
  
  print_indent(indent);
  printf("%s (%u): name '%s', code %d, attr cnt %lld\n", dwarf_id2str(dwarf_tags, tag), (unsigned)tag, name, abbrev_code, (long long int)attr_count);
  
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
      if ( dwarf_formudata(attr_list[i], &uval, &err) == DW_DLV_OK )
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
    show_die(indent, die);
    
   if (dwarf_child(die, &child_die, &err) == DW_DLV_OK)
   {
     if ( dfs_die(dbg, indent+1, child_die) == 0 )
       return 0;
   }
   
    next_die = NULL;
    r = dwarf_siblingof(dbg, die, &next_die, &err);
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

int show_dwarf(Elf *elf)
{
  Dwarf_Error err;
  Dwarf_Debug dbg;
  Dwarf_Die die;
  Dwarf_Signed srcfile_cnt = 0;
  char **srcfile_list = 0;

  if (dwarf_elf_init(elf, DW_DLC_READ, NULL, NULL, &dbg, &err) != DW_DLV_OK)
    return fprintf(stderr, "dwarf_init: %s\n", dwarf_errmsg(err) ), 0;
  
  /* get the first unit */
  if (dwarf_next_cu_header(dbg, NULL, NULL, NULL, NULL, NULL, &err) != DW_DLV_OK)
    return fprintf(stderr, "dwarf_next_cu_header: %s\n", dwarf_errmsg(err) ), 0;
  
  /* get first DIE */
  if (dwarf_siblingof(dbg, NULL, &die, &err) != DW_DLV_OK)
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
  return 1;
}


int main(int argc, char **argv)
{
  int fd = -1;
  Elf *elf = NULL;
  char *elf_filename = NULL;
  if ( argc < 2 )
  {
    printf("%s <input.elf>\n", argv[0]);
    return 1;
  }

  if ( elf_version( EV_CURRENT ) == EV_NONE )
    return fprintf(stderr, "Incorrect libelf version: %s\n", elf_errmsg(-1) ), 0;
  
  elf_filename = argv[1];
  fd = open( elf_filename, O_RDONLY , 0);
  if ( fd >= 0 )
  {
    if (( elf = elf_begin( fd , ELF_C_READ, NULL )) != NULL )
    {
      if ( show_dwarf(elf) )
      {
        elf_end(elf); 
        close(fd);  
        return 0;
      }
      else
      {
        fprintf(stderr, "Conversion failed\n");
      }
      elf_end(elf);
    }
    else
    {
      fprintf(stderr, "elf_begin failed: %s\n", elf_errmsg(-1));
    }
    close(fd);
  }
  else
  {
    perror(elf_filename);
  }
  return 0;
}
