template<class T>
const T& min(const T& a, const T& b)
{
    return (b < a) ? b : a;
}