#include<iostream>
#include<math.h>

using namespace std;
// đa thức tối giản m(x)
int m[10]={0,0,0,0,0,0,1,0,0,1};
int XOR(int a , int b){
    if(a==b) return 0;
    else return 1;
}
// hàm chuyển đồi từ số nguyên sang mảng nhị phân 10 phần tử 
int* change(int a){
    int* b = new int[10];
    for(int i=0;i<10;i++){
        b[i]=0;
    }
    int i=9;
    while(a>0){
        b[i]=a%2;
        a/=2;
        i--;
    }
    return b;
}
// hàm dịch bit từ trái qua phải và mod
void shift(int *a){
    int b =a[0];
    for(int i=8;i>0;i--){
        a[i-1]=a[i];
    }
    a[9]=0;
    if(b ==1 ){
        for(int i=0;i<=9;i++){
            a[i]=XOR(a[i],m[i]);
        }
    }
//    return a;    
}
void shift11(int *a){
    int b =a[0];
    for(int i=10;i>0;i--){
        a[i-1]=a[i];
    }
    a[10]=0;
    
//    return a;    
}
// hàm khi nhân 2 mảng và mod với đa thức tối giản m(x) : a . b = d
int *xmod(int a[], int b[]){
    int c[10]={0,0,0,0,0,0,0,0,0,0};
    int *d = new int[10];
    // gán C = A
    for(int i=0;i<=9;i++){
        c[i]=a[i];
        d[i]=a[i];
    }
    
    for(int i=8;i>=0;i--){
        shift(c);
        if(b[i]==1){
            for(int j=0;j<=9;j++){
                d[j]=XOR(d[j],c[j]);
            }
        }
    }
    if(b[9]==1){
        for(int i=0;i<=9;i++){
            d[i]=XOR(d[i],a[i]);
        }
    }
    return d;
} 

// hàm tìm bậc cao nhất (số bit 1 từ trái sang phải)
int degree(int a[]){
    for(int i=0;i<10;i++){
        if(a[i]==1) return 9-i; // Trả về bậc thật sự của đa thức
    }
    return -1; // đa thức 0
}
// hàm chia đa thức a(x) cho b(x) trong GF(2^10) và trả về thương và dư
// chia lấy dư: a(x) = b(x)*q(x) + r(x)
int* divide(int a[], int b[], int* quotient){
    int *r = new int[10];
    quotient = new int[10]; // mảng lưu số thương
    
    // Khởi tạo
    for(int i=0;i<10;i++){
        r[i]=a[i];
        quotient[i]=0;
    }
    
    int deg_b = degree(b);
    if(deg_b == -1){
        // b = 0, không thể chia cho 0
        return r;
    }
    
    int deg_r = degree(r);
    
    // Thực hiện phép chia
    while(deg_r >= deg_b && deg_r != -1){
        int shift_amount = deg_r - deg_b;
        
        // Đặt bit trong quotient 
        quotient[9 - shift_amount] = 1; // Chỉnh sửa để phù hợp với định nghĩa bậc
        
        // Trừ (XOR) r với b dịch lên
        for(int i=0 ; i<=shift_amount; i++){
            shift(b);// dịch b lên shift_amount bit
        }
        for(int i=0;i<10;i++){
            r[i] = XOR(r[i], b[i]);
            }
        deg_r = degree(r);
    }
    
    return r;
}
int degree11(int a[]){
    for(int i=0;i<11;i++){
        if(a[i]==1) return 10-i; // Trả về bậc thật sự của đa thức
    }
    return -1; // đa thức 0
}
// hàm chia đa thức a(x) cho b(x) trong GF(2^10) với mảng 11 phần tử
// Kết quả trả về: thương và dư (mỗi mảng 10 phần tử)
void divide_11( int b[], int quotient[], int remainder[]){
    //b: số bị chia, quotient: mảng lưu số thương, remainder: mảng lưu số dư
    int r[11]={1,0,0,0,0,0,0,1,0,0,1};
    // Khởi tạo thương
    for (int i = 0; i < 10; i++) {
        quotient[i] = 0;
    }

    int deg_b = degree11(b);
    if (deg_b == -1) {
        // b = 0, không thể chia cho 0
        for (int i = 9; i >=0; i--) {
            remainder[i] = r[i];
        }
        return;
    }

    int deg_r = 10; // Vì mảng r có 11 phần tử, bậc lớn nhất là 10

    // Thực hiện phép chia
    while (deg_r >= deg_b) {
        int shift_amount = deg_r - deg_b;

        // Đặt bit trong quotient
        quotient[10 - shift_amount - 1] = 1; // Chỉnh sửa để phù hợp với định nghĩa bậc
        for(int i=0;i<shift_amount;i++)
            shift11(b);// dịch b lên 1 bit
        // Trừ (XOR) r với b dịch lên
        for (int i = 0; i < 11; i++) {
            r[i] = XOR(r[i], b[i]);
        }
        deg_r = degree11(r);
    }
    // Sao chép phần dư vào remainder (10 phần tử cuối cùng của r)
    for (int i = 9; i >=0; i--) {
        remainder[i] = r[i];
    }
}

// hàm tìm nghịch đảo nhân sử dụng Extended Euclidean Algorithm
// Tìm a sao cho a*b ≡ 1 (mod m(x))
void modularInverse(int b[], int inverse[]){
    int u[11] = {0};
    int v[11] = {0};
    int g1[10] = {0};
    int g2[10] = {0};

    // Khởi tạo: u = b, v = m(x), g1 = 1, g2 = 0
    for (int i = 0; i < 10; i++) {
        u[i + 1] = b[i]; // u là mảng 11 phần tử, b là mảng 10 phần tử
        v[i + 1] = m[i];
    }
    v[0] = 1; // Thêm x^10 vào v
    g1[9] = 1; // g1 = 1 (đa thức hằng số, bậc 0 ở vị trí 9)

    while (true) {
        int quotient[10] = {0};
        int remainder[10] = {0};

        // Chia u cho v, lấy thương và dư
        divide_11(v, quotient, remainder);

        // Nếu dư là 0, kết thúc
        bool isZero = true;
        for (int i = 0; i < 10; i++) {
            if (remainder[i] != 0) {
                isZero = false;
                break;
            }
        }
        if (isZero) break;

        // Cập nhật u và v
        for (int i = 0; i < 11; i++) {
            u[i] = v[i];
            v[i] = 0;
        }
        for (int i = 0; i < 10; i++) {
            v[i + 1] = remainder[i];
        }

        // Cập nhật g1 và g2
        int temp[10] = {0};
        for (int i = 0; i < 10; i++) {
            temp[i] = g1[i];
            g1[i] = g2[i];
        }
        for (int i = 0; i < 10; i++) {
            g2[i] = XOR(temp[i], quotient[i]);
        }
    }

    // Kết quả là g1
    for (int i = 0; i < 10; i++) {
        inverse[i] = g1[i];
    }
}

int main(){
    int a[10] = {0};
    int inverse[10] = {0};

    // Chuyển đổi số 523 thành mảng nhị phân
    int* binary = change(523);
    for (int i = 0; i < 10; i++) {
        a[i] = binary[i];
    }

    // Tìm nghịch đảo nhân
    modularInverse(a, inverse);

    // In kết quả
    cout << "Nghịch đảo của 523 trong GF(2^10) là: ";
    for (int i = 0; i < 10; i++) {
        cout << inverse[i] << " ";
    }
    cout << endl;

    delete[] binary;
    return 0;
}