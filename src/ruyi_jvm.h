//
//  ruyi_jvm.h
//  ruyi
//
//  Created by Songli Huang on 2020/2/15.
//  Copyright © 2020 Songli Huang. All rights reserved.
//

#ifndef ruyi_jvm_h
#define ruyi_jvm_h

#include "ruyi_jvm_classfile.h"
#include "ruyi_code_generator.h"

JVM_ClassFile * ruyi_jvm_cg_file_to_class_file(const ruyi_cg_file* cg_file);

void ruyi_jvm_class_file_destroy(JVM_ClassFile * class_file);


#endif /* ruyi_jvm_h */
