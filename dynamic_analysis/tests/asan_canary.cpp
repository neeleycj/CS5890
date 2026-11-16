// Sanity check run by dyn_analysis.sh before the real tests: this program has
// a deliberate one-element heap-buffer-overflow. If ASan does not report it,
// the toolchain is not instrumenting correctly and "0 ASan reports" from the
// real tests would be meaningless.
int main()
{
    int* a = new int[4];
    int r = a[4]; // out of bounds by one element
    delete[] a;
    return r;
}
