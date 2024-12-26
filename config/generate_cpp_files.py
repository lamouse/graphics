import yaml
import re
import os
import sys

# 获取输入和输出目录
input_directory = sys.argv[1] if len(sys.argv) > 1 else '.'
output_directory = sys.argv[2] if len(sys.argv) > 2 else 'config'

# 创建输出目录
os.makedirs(output_directory, exist_ok=True)

# 读取 YAML 文件
config_path = os.path.join(input_directory, 'config.yaml')
with open(config_path, 'r') as file:
    config = yaml.safe_load(file)

# 读取原始 YAML 文件内容（包含注释）
with open(config_path, 'r') as file:
    raw_content = file.read()

# 提取注释中的类型信息
def get_type_from_comment(struct_name, field):
    pattern = re.compile(rf'{field}:.*?#.*\{{type:\s*(\w+)\}}')
    match = pattern.search(raw_content)
    if not match:
        return None
    return match.group(1)

# 映射 YAML 数据类型到 C++ 数据类型
type_mapping = {
    'int': 'int',
    'string': 'std::string',
    'double': 'double',
    'bool': 'bool',
    'struct': 'struct',
    # 添加更多类型映射
}

def to_snake_case(name):
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1).lower()

def generate_struct_code(struct_name, fields, original_name=None, parent_name=None):
    if original_name is None:
        original_name = struct_name
    struct_code = f'struct {struct_name} {{\n'
    read_code = f'{struct_name} {struct_name}::read_config(const YAML::Node& node) {{\n'
    read_code += f'    {struct_name} config;\n'

    sub_struct_codes = []
    sub_read_codes = []

    for field_name, subfields in fields.items():
        field_type = get_type_from_comment(struct_name if parent_name is None else parent_name, field_name)
        if field_type == 'struct':
            sub_struct_name = f'{field_name.capitalize()}Config'
            sub_struct_code, sub_read_code = generate_struct_code(sub_struct_name, subfields, field_name, struct_name)
            sub_struct_codes.append(sub_struct_code)
            sub_read_codes.append(sub_read_code)
            struct_code += f'    {sub_struct_name} {field_name};\n'
            read_code += f'    config.{field_name} = {sub_struct_name}::read_config(node["{field_name}"]);\n'
        elif field_type:
            cpp_type = type_mapping.get(field_type)
            if not cpp_type:
                raise ValueError(f'Unsupported type: {field_type} for field: {field_name}')
            struct_code += f'    {cpp_type} {field_name};\n'
            read_code += f'    config.{field_name} = node["{field_name}"].as<{cpp_type}>();\n'
        else:
            raise ValueError(f'No type specified for field: {field_name}')

    struct_code += f'\n    static {struct_name} read_config(const YAML::Node& node);\n'
    struct_code += f'    static const char* node_name() {{ return "{original_name}"; }}\n'  # 添加获取节点名称的静态函数
    struct_code += '};\n\n'

    read_code += '    return config;\n'
    read_code += '}\n\n'

    # 确保子结构体在父结构体之前声明
    struct_code = ''.join(sub_struct_codes) + struct_code
    read_code += ''.join(sub_read_codes)

    return struct_code, read_code

# 生成 C++ struct 和读取配置代码
header_codes = []
source_codes = []
for struct_name, fields in config.items():
    struct_name_with_config = f'{struct_name.capitalize()}Config'
    snake_case_name = to_snake_case(struct_name_with_config)

    header_code = f'// This file is auto-generated. Do not edit.\n\n'
    header_code += '#pragma once\n\n'
    header_code += '#define YAML_CPP_API\n'
    header_code += '#include <string>\n#include <yaml-cpp/yaml.h>\n\n'
    header_code += 'namespace config {\n\n'

    struct_code, read_code = generate_struct_code(struct_name_with_config, fields, struct_name)
    header_code += struct_code
    header_code += '} // namespace config\n\n'
    header_codes.append((header_code, snake_case_name))

    source_code = f'#include "{snake_case_name}.h"\n\n'
    source_code += 'namespace config {\n\n'
    source_code += read_code
    source_code += '} // namespace config\n\n'
    source_codes.append((source_code, snake_case_name))

# 写入头文件和源文件
for header_code, snake_case_name in header_codes:
    header_file_path = os.path.join(output_directory, f'{snake_case_name}.h')
    with open(header_file_path, 'w') as file:
        file.write(header_code)

for source_code, snake_case_name in source_codes:
    source_file_path = os.path.join(output_directory, f'{snake_case_name}.cpp')
    with open(source_file_path, 'w') as file:
        file.write(source_code)
