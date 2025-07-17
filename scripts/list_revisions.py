#!/usr/bin/env python3
import yaml
from pathlib import Path

def get_board_revisions():
    project_dir = Path(__file__).parent.parent.absolute()
    board_yml_path = project_dir / "boards" / "stowood" / "db1" / "board.yml"
    
    with open(board_yml_path, 'r') as f:
        board_config = yaml.safe_load(f)
    
    revisions = board_config['board']['revision']['revisions']
    return [revision['name'] for revision in revisions]

if __name__ == "__main__":
    revisions = get_board_revisions()
    for revision in revisions:
        print(revision)