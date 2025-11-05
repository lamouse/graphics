# gen_asset_hashes.py
import sys
import json
import xxhash
import os

def hash_file(filepath):
    with open(filepath, 'rb') as f:
        return xxhash.xxh3_64(f.read()).intdigest()

def main(output_json, list_file):
    # 获取 assets.txt 所在目录（作为基准目录）
    base_dir = os.path.dirname(os.path.abspath(list_file))
    assets = []

    with open(list_file, 'r', encoding='utf-8') as f:
        lines = f.read().splitlines()

    for line in lines:
        line = line.strip()
        if not line or line.startswith('#'):
            continue

        # 将 assets.txt 中的路径视为相对于它自己所在目录
        abs_path = os.path.join(base_dir, line)
        if not os.path.isfile(abs_path):
            print(f"Warning: {abs_path} not found, skipping.", file=sys.stderr)
            continue

        h = hash_file(abs_path)
        # 在 JSON 中存储**相对于 base_dir 的原始路径**（或你想要的格式）
        assets.append({
            "path": line.replace('\\', '/'),  # 保持 assets.txt 中的原始相对路径
            "hash": h
        })

    # 确保输出目录存在
    os.makedirs(os.path.dirname(output_json), exist_ok=True)

    with open(output_json, 'w', encoding='utf-8') as f:
        json.dump({"assets": assets}, f, indent=2)
    print(f"Generated {output_json} with {len(assets)} assets.")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python gen_asset_hashes.py <output.json> <file_list.txt>")
        sys.exit(1)

    output_json = sys.argv[1]
    list_file = sys.argv[2]
    main(output_json, list_file)