contract C {
    function external_test_function() external {}
    function internal_test_function() internal {}

    function comparison_operator_between_internal_and_external_function_pointers() external returns (bool) {
        function () external external_function_pointer_local = this.external_test_function;
        function () internal internal_function_pointer_local = internal_test_function;

        assert(
            this.external_test_function == external_function_pointer_local &&
            internal_function_pointer_local == internal_test_function
        );
        assert(
            internal_function_pointer_local != external_function_pointer_local &&
            internal_test_function != this.external_test_function
        );

        return true;
    }
}
// ----
// Warning 3075: (509-566): Comparison of internal function pointers can yield unexpected results in the legacy pipeline with the optimizer enabled, and will be disallowed entirely in the next breaking release.
// TypeError 2271: (606-672): Built-in binary operator != cannot be applied to types function () and function () external.
// TypeError 2271: (688-741): Built-in binary operator != cannot be applied to types function () and function () external.
