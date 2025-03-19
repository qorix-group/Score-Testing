import re
import os
import json
import logging
from typing import List, Dict, Set, Tuple, Optional


# Configure logging
def setup_logging(log_file: str):
    """Set up detailed logging"""
    logging.basicConfig(
        filename=log_file,
        level=logging.DEBUG,
        format="%(asctime)s - %(levelname)s - %(message)s",
    )
    logging.info("Logging initialized")


# Patterns
METHOD_PATTERN = re.compile(
    r"^(\s*)([\w\<\>:\s\*&]+)\s+(\w+::[^\s(]+)\s*\(([^)]*)\)\s*(const)?\s*(\{?)\s*$",
    re.MULTILINE
)

FUNCTION_PATTERN = re.compile(
    r"^(\s*)(?!if|while|for|switch|catch|return)([\w\<\>:\s\*&]+)\s+([^\s(]+)\s*\(([^)]*)\)\s*(const)?\s*(\{?)\s*$",
    re.MULTILINE
)

CONDITION_PATTERN = re.compile(
    r"^\s*(if|else if|else)\s*(\(.*\))?\s*\{?",
    re.IGNORECASE
)

RETURN_PATTERN = re.compile(
    r"^\s*return\s+(.+?)\s*;",
    re.IGNORECASE
)


def parse_cpp_file(file_path: str) -> Tuple[List[Dict], Set[int]]:
    """Parse C++ files with detailed scope tracking"""
    logging.debug(f"Parsing {file_path}")

    with open(file_path, "r") as f:
        content = f.read()
    lines = content.split('\n')
    unprocessed = set(range(1, len(lines) + 1))
    elements = []
    current_function = None
    bracket_depth = 0

    for line_num, line in enumerate(lines, 1):
        raw_line = line.rstrip()
        logging.debug(f"Processing line {line_num}: {raw_line}")

        # Track function scope
        if current_function:
            # Update bracket depth
            bracket_depth += line.count('{') - line.count('}')

            # Detect end of function
            if bracket_depth <= 0:
                logging.debug(f"Exiting function {current_function['name']}")
                current_function = None
                bracket_depth = 0
                continue

            # Detect conditions within function
            cond_match = CONDITION_PATTERN.match(raw_line)
            if cond_match:
                cond_type = cond_match.group(1).lower()
                condition = cond_match.group(2).strip('() ') if cond_match.group(2) else ""
                current_function['conditions'].append({
                    'type': cond_type,
                    'condition': condition,
                    'line': line_num
                })
                logging.debug(f"Found condition in {current_function['name']}: {cond_type} {condition}")
                unprocessed.discard(line_num)
                continue

            # Detect returns within function
            return_match = RETURN_PATTERN.match(raw_line)
            if return_match:
                return_value = return_match.group(1).strip()
                current_function['returns'].append({
                    'value': return_value,
                    'line': line_num
                })
                logging.debug(f"Found return in {current_function['name']}: {return_value}")
                unprocessed.discard(line_num)
                continue

        # Detect new functions/methods
        if not current_function:
            match = METHOD_PATTERN.search(raw_line) or FUNCTION_PATTERN.search(raw_line)
            if match:
                groups = match.groups()
                is_method = '::' in groups[3] if METHOD_PATTERN == match.re else False

                current_function = {
                    "type": "method" if is_method else "function",
                    "name": groups[3],
                    "return_type": groups[2].strip(),
                    "parameters": groups[4] if len(groups) > 4 else "",
                    "line": line_num,
                    "conditions": [],
                    "returns": []
                }
                elements.append(current_function)
                bracket_depth = line.count('{')
                unprocessed.discard(line_num)
                logging.info(f"Found {current_function['type']}: {current_function['name']}")
                continue

        # Mark as unprocessed if not matched
        if line_num in unprocessed:
            logging.warning(f"Unprocessed line {line_num}: {raw_line}")

    return elements, unprocessed


def generate_sphinx_needs(logical_structure: List[Dict], impl_file: str, test_file: str):
    """Generate hierarchical documentation with relationships"""
    with open(impl_file, "w") as impl_f, open(test_file, "w") as test_f:
        impl_f.write(".. needs_documentation\n\n")
        test_f.write(".. needs_documentation\n\n")

        for element in logical_structure:
            # Implementation elements
            main_id = f"impl_{element['name'].replace('::', '__')}"
            impl_f.write(
                f".. need:: {main_id}\n"
                f"    :type: {element['type']}\n"
                f"    :name: {element['name']}\n"
                f"    :description: {element['return_type']} {element['name']}({element['parameters']})\n\n"
            )

            # Conditions
            for i, cond in enumerate(element['conditions'], 1):
                cond_id = f"{main_id}_cond_{i}"
                impl_f.write(
                    f".. need:: {cond_id}\n"
                    f"    :type: condition\n"
                    f"    :name: {cond['type']} {cond['condition']}\n"
                    f"    :parent: {main_id}\n\n"
                )

            # Returns
            for i, ret in enumerate(element['returns'], 1):
                ret_id = f"{main_id}_ret_{i}"
                impl_f.write(
                    f".. need:: {ret_id}\n"
                    f"    :type: return\n"
                    f"    :name: {ret['value']}\n"
                    f"    :parent: {main_id}\n\n"
                )

            # Test cases
            test_f.write(
                f".. need:: test_{main_id}\n"
                f"    :type: test\n"
                f"    :name: Test {element['name']}\n"
                f"    :links: {main_id}\n"
            )

            for i in range(len(element['conditions'])):
                test_f.write(
                    f"    :links: {main_id}_cond_{i + 1}\n"
                )

            for i in range(len(element['returns'])):
                test_f.write(
                    f"    :links: {main_id}_ret_{i + 1}\n"
                )

            test_f.write("\n")


def process_directory(source_path: str, output_dir: str):
    """Process files with error handling"""
    logging.info(f"📂 Processing directory: {source_path}")

    for root, _, files in os.walk(source_path):
        for file in files:
            if file.endswith((".cpp", ".h", ".hpp")):
                file_path = os.path.join(root, file)
                try:
                    logging.info(f"🔧 Processing {file_path}")
                    elements, unprocessed = parse_cpp_file(file_path)

                    base_name = os.path.splitext(file)[0]
                    impl_file = os.path.join(output_dir, f"{base_name}_impl.rst")
                    test_file = os.path.join(output_dir, f"{base_name}_test.rst")

                    generate_sphinx_needs(elements, impl_file, test_file)

                except Exception as e:
                    logging.error(f"🔥 Error processing {file_path}: {str(e)}", exc_info=True)


def main(config_file: str):
    """Main entry point"""
    with open(config_file) as f:
        config = json.load(f)

    setup_logging(config["log_file"])
    process_directory(config["source_path"], config["output_dir"])
    logging.info("🏁 Processing complete")


if __name__ == "__main__":
    try:
        main("config.json")
    except Exception as e:
        logging.critical(f"💥 Critical failure: {str(e)}", exc_info=True)
        raise