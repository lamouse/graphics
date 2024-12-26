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

def generate_struct_and_namespace_code(struct_name, fields, parent_name=None):
    struct_code = f'struct {struct_name.capitalize()} {{\n'
    read_code = f'{struct_name.capitalize()} {struct_name.capitalize()}::read_config(const YAML::Node& node);\n'

    sub_struct_definitions = []

    for field_name, subfields in fields.items():
        field_type = get_type_from_comment(struct_name if parent_name is None else parent_name, field_name)
        if field_type == 'struct':
            sub_struct_name = f'{field_name.capitalize()}'
            sub_struct_code, sub_read_code = generate_struct_and_namespace_code(field_name, subfields, struct_name)
            sub_struct_definitions.append(sub_struct_code)
            struct_code += f'    {field_name.lower()}::{sub_struct_name} {field_name};\n'
        else:
            struct_code += f'    {type_mapping.get(field_type, "unknown")} {field_name};\n'

    struct_code += f'\n    static {struct_name.capitalize()} read_config(const YAML::Node& node);\n'
    struct_code += f'    static const char* node_name() {{ return "{struct_name}"; }}\n'
    struct_code += '};\n\n'

    namespace_code = f'namespace {struct_name.lower()} {{\n'
    namespace_code += ''.join(sub_struct_definitions)
    namespace_code += struct_code
    namespace_code += '} // namespace ' + struct_name.lower() + '\n\n'

    return namespace_code, read_code

def generate_source_code(struct_name, fields, parent_name=None):
    read_code = f'{struct_name.capitalize()} {struct_name.capitalize()}::read_config(const YAML::Node& node) {{\n'
    read_code += f'    {struct_name.capitalize()} config;\n'

    sub_read_definitions = []

    for field_name, subfields in fields.items():
        field_type = get_type_from_comment(struct_name if parent_name is None else parent_name, field_name)
        if field_type == 'struct':
            sub_struct_code = generate_source_code(field_name, subfields, struct_name)
            sub_read_definitions.append(sub_struct_code)
            read_code += f'    config.{field_name} = {field_name.lower()}::{field_name.capitalize()}::read_config(node["{field_name}"]);\n'
        else:
            read_code += f'    config.{field_name} = node["{field_name}"].as<{type_mapping.get(field_type, "unknown")}>();\n'

    read_code += '    return config;\n'
    read_code += '}\n\n'

    if parent_name:
        return f'namespace {struct_name.lower()} {{\n\n' + read_code + f'}} // namespace {struct_name.lower()}\n\n' + ''.join(sub_read_definitions)
    else:
        return f'namespace {struct_name.lower()} {{\n\n' + read_code + ''.join(sub_read_definitions) + f'}} // namespace {struct_name.lower()}\n\n'

# 生成 C++ 命名空间和读取配置代码
header_codes = []
source_codes = []
for struct_name, fields in config.items():
    snake_case_name = to_snake_case(struct_name)

    header_code = f'// This file is auto-generated. Do not edit.\n\n'
    header_code += '#pragma once\n\n'
    header_code += '#define YAML_CPP_API\n'
    header_code += '#include <string>\n#include <yaml-cpp/yaml.h>\n\n'
    header_code += 'namespace config {\n\n'

    namespace_code, _ = generate_struct_and_namespace_code(struct_name, fields)
    header_code += namespace_code
    header_code += '} // namespace config\n\n'
    header_codes.append((header_code, snake_case_name))

    source_code = f'#include "{snake_case_name}.h"\n\n'
    source_code += 'namespace config {\n\n'
    source_code += generate_source_code(struct_name, fields)
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
