//
//  ruyi_jvm.c
//  ruyi
//
//  Created by Songli Huang on 2020/2/15.
//  Copyright © 2020 Songli Huang. All rights reserved.
//

#include "ruyi_jvm.h"
#include "ruyi_mem.h"
#include "ruyi_basics.h"


JVM_ClassFile * ruyi_jvm_cg_file_to_class_file(const ruyi_cg_file* cg_file) {
    JVM_ClassFile * class_file = (JVM_ClassFile *) ruyi_mem_alloc(sizeof(JVM_ClassFile));
    class_file->magic = JVM_CLASSFILE_MAGIC;
    class_file->major_version = JVM_CLASSFILE_MAJOR_VERSION;
    class_file->minor_version = JVM_CLASSFILE_MINOR_VERSION;
    // TODO
    
    return class_file;
ruyi_jvm_cg_file_to_class_file_error:
    if (class_file) {
        ruyi_jvm_class_file_destroy(class_file);
    }
    return NULL;
}



static void constant_pool_destroy(JVM_cp_info *cp) {
    if (!cp) {
        return;
    }
    // how to release info
    if (cp->info) {
        // TODO fix the info release way !!!
        ruyi_mem_free(cp->info);
    }
}

static void attribute_info_destroy(JVM_attribute_info *attr_info) {
    if (!attr_info) {
        return;
    }
    if (attr_info->attribute_length > 0) {
        // TODO fix the info release way !!!
        ruyi_mem_free(attr_info->info);
    }
}

static void field_info_destroy(JVM_field_info *field_info) {
    u2 i;
    if (!field_info) {
        return;
    }
    if (field_info->attributes_count > 0) {
        for (i = 0; i < field_info->attributes_count; i++) {
            attribute_info_destroy(&field_info->attributes[i]);
        }
        ruyi_mem_free(field_info->attributes);
    }
}

static void method_info_destroy(JVM_method_info *method_info) {
    u2 i;
    if (!method_info) {
        return;
    }
    if (method_info->attributes_count > 0) {
        for (i = 0; i < method_info->attributes_count; i++) {
            attribute_info_destroy(&method_info->attributes[i]);
        }
        ruyi_mem_free(method_info->attributes);
    }
}

void ruyi_jvm_class_file_destroy(JVM_ClassFile * class_file) {
    u2 i;
    if (!class_file) {
        return;
    }
    if (class_file->constant_pool_count > 0) {
        for (i = 0; i < class_file->constant_pool_count; i++) {
            constant_pool_destroy(&class_file->constant_pool[i]);
        }
        ruyi_mem_free(class_file->constant_pool);
    }
    if (class_file->fields_count > 0) {
        for (i = 0; i < class_file->fields_count; i++) {
            field_info_destroy(&class_file->fields[i]);
        }
        ruyi_mem_free(class_file->fields);
    }
    if (class_file->methods_count > 0) {
        for (i = 0; i < class_file->methods_count; i++) {
            method_info_destroy(&class_file->methods[i]);
        }
        ruyi_mem_free(class_file->methods);
    }
    if (class_file->attributes_count > 0) {
        for (i = 0; i < class_file->attributes_count; i++) {
            attribute_info_destroy(&class_file->attributes[i]);
        }
        ruyi_mem_free(class_file->attributes);
    }
    
    ruyi_mem_free(class_file);
}

static JVM_cp_info* ruyi_get_cp_from_ir_file(const ruyi_cg_file *ir_file) {
    UINT32 i, len;
    ruyi_cg_file_const_pool *cp;
    len = ir_file->cp_count;
    for (i = 0; i < len; i++) {
        cp = ir_file->cp[i];
        //
        // TODO !!!!
    }
    return NULL;
}
