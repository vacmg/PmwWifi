from pytest_embedded_qemu import QemuDut

def test_qemu_run_unity(dut: QemuDut) -> None:
    dut.run_all_single_board_cases()
    assert len(dut.testsuite.testcases) > 0, "No test cases found"
