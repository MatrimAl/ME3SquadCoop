"""Direct IP transport configuration and network unit tests."""
from pathlib import Path
import sys
import unittest

HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))

from test_direct_ip_config import DirectIPTests

__all__ = ["DirectIPTests"]

if __name__ == "__main__":
    unittest.main()
