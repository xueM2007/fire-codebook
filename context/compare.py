import sys

def compare_files(file1, file2):
    """
    比较两个文件的内容是否一致。
    """
    with open(file1, 'r') as f1, open(file2, 'r') as f2:
        line_num = 0
        for line1, line2 in zip(f1, f2):
            line_num += 1
            # 去掉行尾的换行符，并按空格分割成列表
            nums1 = line1.strip().split()
            nums2 = line2.strip().split()

            # 检查每行的数值数量是否一致
            if len(nums1) != len(nums2):
                print(f"Error: Line {line_num} has different number of values.")
                print(f"File 1: {len(nums1)} values, File 2: {len(nums2)} values.")
                return False

            # 逐个比较数值
            for i, (num1, num2) in enumerate(zip(nums1, nums2)):
                if num1 != num2:
                    print(f"Error: Line {line_num}, Value {i + 1} is different.")
                    print(f"File 1: {num1}, File 2: {num2}")
                    return False

        # 检查文件行数是否一致
        if f1.readline() or f2.readline():
            print("Error: Files have different number of lines.")
            return False

    # 如果所有行都一致
    print("The two files are identical.")
    return True

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python compare_outputs.py <file1> <file2>")
        sys.exit(1)

    file1 = sys.argv[1]
    file2 = sys.argv[2]

    if compare_files(file1, file2):
        sys.exit(0)  # 文件一致，返回 0
    else:
        sys.exit(1)  # 文件不一致，返回 1