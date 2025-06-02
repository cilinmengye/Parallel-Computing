#include <torch/extension.h>
#include <ATen/ATen.h>
#include <iostream>
#include <time.h>
#include <sys/time.h>
#include <vector>
#include <immintrin.h>
#include <algorithm>

// Uncomment for ISPC
//#include "module_ispc.h"
//using namespace ispc;

// ------------------------------------ //
// 	WARM-UP: ACCESSING TENSORS      //
// ------------------------------------ //

// Step #1: Understand Read/Write Accessors for a 2D Tensor
inline float twoDimRead(std::vector<float> &tensor, int &x, int &y, const int &sizeX) {
    // Note that sizeX is the size of a Row, not the number of rows
    return tensor[x * (sizeX)+ y];
}

inline void twoDimWrite(std::vector<float> &tensor, int &x, int &y, const int &sizeX, float &val) {
    tensor[x * (sizeX) + y] = val;
}

// Step #2: Implement Read/Write Accessors for a 4D Tensor
inline float fourDimRead(std::vector<float> &tensor, int &x, int &y, int &z, int &b, 
        const int &sizeX, const int &sizeY, const int &sizeZ) {
    int yz = sizeY * sizeZ;
    int xyz = sizeX * yz;
    return tensor[x * xyz + y * yz + z * sizeZ + b];
}

inline void fourDimWrite(std::vector<float> &tensor, int &x, int &y, int &z, int &b, 
        const int &sizeX, const int &sizeY, const int &sizeZ, float &val) {
    int yz = sizeY * sizeZ;
    int xyz = sizeX * yz;
    tensor[x * xyz + y * yz + z * sizeZ + b] = val;
}

// DO NOT EDIT THIS FUNCTION //
std::vector<float> formatTensor(torch::Tensor tensor) {
    tensor = tensor.flatten();
    tensor = tensor.contiguous();
    std::vector<float> vec(tensor.data_ptr<float>(), tensor.data_ptr<float>() + tensor.numel());
    return vec;
}

/* Programming Your Attention Modules.
 * 
 * You are given Q, K, and V Tensors as inputs that are formatted as vectors. We have also created O and QK^t Tensors 
 * that are formatted as vectors. After you have implemented your accessors in the Warm-Up you should be able to
 * read/write to these tensors via the read/write functions above.
 *
 * You are also given 4 integers as parameters: B, H, N, d:
 *
 * B (Batch Size) - The number of samples for your attention layer. Think of it this way - if I asked my dnn
 * a question and it output 5 different answers it had a batch size of 5. These samples are independent of each
 * other and thus can be parallelized.
 *
 * H (Number of Heads) - Each head runs on its own set of Q, K, V matrices. This effectively allows each head
 * to operate the same attention algorithm, but each with each head using different hyperparameters. These
 * allow each head to have their own definition of what relevance is when looking at a token. These heads
 * can operate independently of one another and thus can be parallized.
 *
 * N (Sequence Length) - The number of tokens. You may think of this as the number of words in a sample.
 *
 * d (Embedding Dimensionality) - The number of features each token encodes per attention head. Let's
 * say I encoded a word using the follow (length, number of vowels, has a capital letters). The
 * emvedded dimensionaliy would be 3.
 * */

// ---------------------------------------------------------- //
//                  PART 1: NAIVE ATTENTION                   //
// ---------------------------------------------------------- //

torch::Tensor myNaiveAttention(torch::Tensor QTensor, torch::Tensor KTensor, torch::Tensor VTensor, torch::Tensor QK_tTensor,
                int B, int H, int N, int d){

    // Q, K, V are passed in with Shape: (B, H, N, d)
    //QK^t Intermediate Tensor has Shape (N, N)
    
    //Make O Tensor with Shape (B, H, N, d) 
    at::Tensor OTensor = at::zeros({B, H, N, d}, at::kFloat);

    //Format O, Q, K, and V tensors into 4D vectors
    std::vector<float> O = formatTensor(OTensor);
    std::vector<float> Q = formatTensor(QTensor);
    std::vector<float> K = formatTensor(KTensor);
    std::vector<float> V = formatTensor(VTensor);

    //Format QK_t Tensor into a 2D vector.
    std::vector<float> QK_t = formatTensor(QK_tTensor);
    
    /* Here is an example of how to read/write 0's to  Q (B, H, N, d) using the 4D accessors

        //loop over Batch Size
         for (int b = 0; b < B; b++) {

             //loop over Heads
             for (int h = 0; h < H; h++) {

                 //loop over Sequence Length
                 for (int i = 0; i < N; i++) {

                     //loop over Embedding Dimensionality
                     for (int j = 0; j < d; j++) {
                        float val = fourDimRead(Q, b, h, i, j, H, N, d);
                        val = 0.0;
                        fourDimWrite(Q, b, h, i, j, H, N, d, val);
                     }
                 }
             }
         }
    */

    /* Here is an example of how to read/write 0's to  QK_t (N, N) using the 2D accessors

           for (int i = 0; i < N; i++) {
	       for (int j = 0; j < N; j++) {
	           float val = twoDimRead(QK_t, i, j, N);
               val = 0.0;
	           twoDimWrite(QK_t, i, j, N, val);
             }
         }
    */
    
    // -------- YOUR CODE HERE  -------- //
    
    for (int b = 0; b < B; b++) {
        for (int h = 0; h < H; h++) {
            // get QK_t
            for (int i = 0; i < N; i++) { // Q的行
                for (int j = 0; j < N; j++) { // K的行
                    float val = 0;
                    for (int k = 0; k < d; k++) { // Q,K的列
                        float wq = fourDimRead(Q, b, h, i, k, H, N, d);
                        float wk = fourDimRead(K, b, h, j, k, H, N, d);
                        val += wq * wk;
                    }
                    twoDimWrite(QK_t, i, j, N, val);
                }
            }
            // softmax(QK_t)
            for (int i = 0; i < N; i++) { // QK_t的行
                float rowE = 0; // 一行元素exp的总值
                for (int j = 0; j < N; j++) { // QK_t的列
                    rowE += exp(twoDimRead(QK_t, i, j, N));
                }
                for (int j = 0; j < N; j++) {
                    float val = exp(twoDimRead(QK_t, i, j, N)) / rowE;
                    twoDimWrite(QK_t, i, j, N, val); 
                }
            }
            //  multiply QK^t with V
            for (int i = 0; i < N; i++) { // QK_t的行
                for (int j = 0; j < d; j++) { // V的列
                    float val = 0;
                    for (int k = 0; k < N; k++) { // QK_t的列，V的行
                        float wqk_t = twoDimRead(QK_t, i, k, N);
                        float wv = fourDimRead(V, b, h, k, j, H, N, d);
                        val += wqk_t * wv;
                    }
                    fourDimWrite(O, b, h, i, j, H, N, d, val);
                }
            }
        }
    }
    
    // DO NOT EDIT THIS RETURN STATEMENT //
    // It formats your C++ Vector O back into a Tensor of Shape (B, H, N, d) and returns it //
    return torch::from_blob(O.data(), {B, H, N, d}, torch::TensorOptions().dtype(torch::kFloat32)).clone();
}


// ---------------------------------------------------------- //
//     PART 2: BLOCKED MATRIX MULTIPLY AND UNFUSED SOFTMAX    //
// ---------------------------------------------------------- //

torch::Tensor myUnfusedAttentionBlocked(torch::Tensor QTensor, torch::Tensor KTensor, torch::Tensor VTensor, torch::Tensor QK_tTensor,
                int B, int H, int N, int d){
    
    // Q, K, V are passed in with Shape: (B, H, N, d)
    //QK^t Intermediate Tensor has Shape (N, N)

    //Make O Tensor with Shape (B, H, N, d) 
    at::Tensor OTensor = at::zeros({B, H, N, d}, at::kFloat);

    //Format O, Q, K, and V tensors into 4D vectors
    std::vector<float> O = formatTensor(OTensor);
    std::vector<float> Q = formatTensor(QTensor);
    std::vector<float> K = formatTensor(KTensor);
    std::vector<float> V = formatTensor(VTensor);

    //Format QK_t Tensor into a 2D vector.
    std::vector<float> QK_t = formatTensor(QK_tTensor);

    // -------- YOUR CODE HERE  -------- //
    int tile_size = 8;
    for (int b = 0; b < B; b++) {
        for (int h = 0; h < H; h++) {
            // get QK_t
            for (int iblock = 0; iblock < N; iblock += tile_size) { // Q的行
                for (int jblock = 0; jblock < N; jblock += tile_size) { // K的行
                    for (int kblock = 0; kblock < d; kblock += tile_size) { // Q,K的列
                        int ibound = std::min(tile_size, N - iblock);
                        int jbound = std::min(tile_size, N - jblock);
                        int kbound = std::min(tile_size, d - kblock);
                        for (int i = 0; i < ibound; i++) { // Q小矩阵的行
                            for (int j = 0; j < jbound; j++) { // K小矩阵的行
                                int in = iblock + i;
                                int jn = jblock + j;
                                float val = twoDimRead(QK_t, in, jn, N);
                                for (int k = 0; k < kbound; k++) { // Q,K小矩阵的列
                                    int kn = kblock + k;
                                    float wq = fourDimRead(Q, b, h, in, kn, H, N, d);
                                    float wk = fourDimRead(K, b, h, jn, kn, H, N, d);
                                    val += wq * wk;
                                }
                                twoDimWrite(QK_t, in, jn, N, val);
                            }
                        }
                    }
                }
            }
            // softmax(QK_t)
            for (int i = 0; i < N; i++) { // QK_t的行
                float rowE = 0; // 一行元素exp的总值
                for (int j = 0; j < N; j++) { // QK_t的列
                    rowE += exp(twoDimRead(QK_t, i, j, N));
                }
                for (int j = 0; j < N; j++) {
                    float val = exp(twoDimRead(QK_t, i, j, N)) / rowE;
                    twoDimWrite(QK_t, i, j, N, val); 
                }
            }
            //  multiply QK^t with V
            for (int iblock = 0; iblock < N; iblock += tile_size) { // QK_t的行
                for (int jblock = 0; jblock < d; jblock += tile_size) { // V的列
                    for (int kblock = 0; kblock < N; kblock += tile_size) { // QK_t的列，V的行
                        int ibound = std::min(tile_size, N - iblock);
                        int jbound = std::min(tile_size, d - jblock);
                        int kbound = std::min(tile_size, N - kblock);
                        for (int i = 0; i < ibound; i++) {
                            for (int j = 0; j < jbound; j++) {
                                int in = iblock + i;
                                int jn = jblock + j;
                                float val = fourDimRead(O, b, h, in, jn, H, N, d);
                                for (int k = 0; k < kbound; k++) {
                                    int kn = kblock + k;
                                    float wqk_t = twoDimRead(QK_t, in, kn, N);
                                    float wv = fourDimRead(V, b, h, kn, jn, H, N, d);
                                    val += wqk_t * wv;
                                }
                                fourDimWrite(O, b, h, in, jn, H, N, d, val);
                            }
                        }
                    }
                }
            }
        }
    }

    // DO NOT EDIT THIS RETURN STATEMENT //
    // It formats your C++ Vector O back into a Tensor of Shape (B, H, N, d) and returns it //
    return torch::from_blob(O.data(), {B, H, N, d}, torch::TensorOptions().dtype(torch::kFloat32)).clone();
}


// ---------------------------------------------------------- //
//                 PART 3: FUSED ATTENTION     	              //
// ---------------------------------------------------------- //

torch::Tensor myFusedAttention(torch::Tensor QTensor, torch::Tensor KTensor, torch::Tensor VTensor, torch::Tensor temp,
                int B, int H, int N, int d){

    // Q, K, V are passed in with Shape: (B, H, N, d)

    //Make O Tensor with Shape (B, H, N, d)
    //and O Row Tensor with Shape (N)
    at::Tensor OTensor = at::zeros({B, H, N, d}, at::kFloat);
    at::Tensor ORowTensor = at::zeros({N}, at::kFloat);

    //Format Y, Q, K, and V tensors into 4D vectors
    std::vector<float> O = formatTensor(OTensor);
    std::vector<float> Q = formatTensor(QTensor);
    std::vector<float> K = formatTensor(KTensor);
    std::vector<float> V = formatTensor(VTensor);
    
    //Format ORow Tensor into a 1D vector
    // You can simply access this as ORow[i]
    std::vector<float> ORow = formatTensor(ORowTensor);

    // -------- YOUR CODE HERE  -------- //
    // We give you a template of the first three loops for your convenience
    #pragma omp parallel for collapse(3)
    //loop over batch
    for (int b = 0; b < B; b++){
        //loop over heads
        for (int h = 0; h < H; h++){
            for (int i = 0; i < N ; i++){
                // YRow is moved inside so each OpenMP thread gets a local copy.
                at::Tensor ORowTensor = temp.index({torch::indexing::Slice(omp_get_thread_num(), torch::indexing::None)});
                // 我这里将其当做2D 1xN使用   
                std::vector<float> ORow = formatTensor(ORowTensor);
		        //YOUR CODE HERE
                // get row of QK_t
                float rowE = 0;
                int zero = 0;
                for (int j = 0; j < N; j++) {
                    float val = 0;
                    for (int k = 0; k < d; k++) {
                        float wq = fourDimRead(Q, b, h, i, k, H, N, d);
                        float wk = fourDimRead(K, b, h, j, k, H, N, d);
                        val += wq * wk;
                    }
                    val = exp(val);
                    rowE += val;
                    twoDimWrite(ORow, zero, j, N, val);
                }
                // softmax
                for (int j = 0; j < N; j++) {
                    float val = twoDimRead(ORow, zero, j, N) / rowE;
                    twoDimWrite(ORow, zero, j, N, val);
                }
                // multiply softmax(row of QK_t) with V
                for (int j = 0; j < d; j++) {
                    float val = 0;
                    for (int k = 0; k < N; k++) {
                        float wqk_t = twoDimRead(ORow, zero, k, N);
                        float wv = fourDimRead(V, b, h, k, j, H, N, d);
                        val += wqk_t * wv;
                    }
                    fourDimWrite(O, b, h, i, j, H, N, d, val);
                }
            }
	    }
    }
	
    // DO NOT EDIT THIS RETURN STATEMENT //
    // It formats your C++ Vector O back into a Tensor of Shape (B, H, N, d) and returns it //
    return torch::from_blob(O.data(), {B, H, N, d}, torch::TensorOptions().dtype(torch::kFloat32)).clone();
}


// ---------------------------------------------------------- //
//                PART 4: FLASH ATTENTION 		      //
// ---------------------------------------------------------- //

torch::Tensor myFlashAttention(torch::Tensor QTensor, torch::Tensor KTensor, torch::Tensor VTensor,
               torch::Tensor QiTensor, torch::Tensor KjTensor, torch::Tensor VjTensor,
               torch::Tensor SijTensor, torch::Tensor PijTensor, torch::Tensor PVTensor,
               torch::Tensor OiTensor, torch::Tensor LTensor,  torch::Tensor LiTensor, 
	       torch::Tensor LijTensor, torch::Tensor LnewTensor, int Bc, int Br,
                int B, int H, int N, int d) {
        
    // Q, K, V are passed in with Shape: (B, H, N, d)
    // Sij, Pij are passed in with Shape: (Br, Bc)
    // Kj, Vj are passed in with Shape: (Bc, d)
    // Qi, Oi, and PV  are passed in with Shape: (Br, d)
    // L in passed in with Shape: (N)
    // Li, Lij, and Lnew are passed in with shape (Br)

    //Make O Tensor with Shape (B, H, N, d)
    at::Tensor OTensor = at::zeros({B, H, N, d}, at::kFloat);
   
    //Format All Tensors into Vectors
    std::vector<float> O = formatTensor(OTensor);
    std::vector<float> Q = formatTensor(QTensor);
    std::vector<float> K = formatTensor(KTensor);
    std::vector<float> V = formatTensor(VTensor);
    std::vector<float> Sij = formatTensor(SijTensor);
    std::vector<float> Pij = formatTensor(PijTensor);
    std::vector<float> Kj = formatTensor(KjTensor);
    std::vector<float> Vj = formatTensor(VjTensor);
    std::vector<float> Qi = formatTensor(QiTensor);
    std::vector<float> Oi = formatTensor(OiTensor);
    std::vector<float> l = formatTensor(LTensor);
    std::vector<float> PV = formatTensor(PVTensor);
    std::vector<float> li = formatTensor(LiTensor);
    std::vector<float> lij = formatTensor(LijTensor);
    std::vector<float> lnew = formatTensor(LnewTensor);

    // -------- YOUR CODE HERE  -------- //
    for (int b = 0; b < B; b++) {
        for (int h = 0; h < H; h++) {
            std::fill(l.begin(), l.end(), 0);
            for (int jblock = 0; jblock < N; jblock += Bc) { // K的行
                // Load Kj(Bcxd)， Vj(BCxd) into local memory blocks
                int jbound = std::min(Bc, N - jblock);
                for (int i = 0; i < jbound; i++) {
                    int in = jblock + i;
                    for (int j = 0; j < d; j++) {
                        float kj = fourDimRead(K, b, h, in, j, H, N, d); twoDimWrite(Kj, i, j, d, kj);
                        float vj = fourDimRead(V, b, h, in, j, H, N, d); twoDimWrite(Vj, i, j, d, vj);
                    }
                }
                for (int iblock = 0; iblock < N; iblock += Br) { // Q的行
                    // Load Qi(Br x d), Oi(Br x d), li(1 x Br) into local memory blocks
                    int ibound = std::min(Br, N - iblock);
                    for (int i = 0; i < ibound; i++) {
                        int in = iblock + i;
                        li[i] = l[in];
                        for (int j = 0; j < d; j++) {
                            float qi = fourDimRead(Q, b, h, in, j, H, N, d); twoDimWrite(Qi, i, j, d, qi);
                            float oi = fourDimRead(O, b, h, in, j, H, N, d); twoDimWrite(Oi, i, j, d, oi);
                        }
                    }
                    // Compute Sij(Br x Bc) <- QiKj_t(Br x Bc) of Size (Br x Bc) via matrix multiply
                    for (int i = 0; i < ibound; i++) { // Qi的行
                        for (int j = 0; j < jbound; j++) { // Kj的行
                            float val = 0;
                            for (int k = 0; k < d; k++) { // Qi, Kj的列
                                float qw = twoDimRead(Qi, i, k, d);
                                float qk = twoDimRead(Kj, j, k, d);
                                val += qw * qk;
                            }
                            twoDimWrite(Sij, i, j, Bc, val);
                        }
                    }
                    // Compute Pij(Br x Bc) <- exp(Sij) of size (Br x Bc)
                    for (int i = 0; i < ibound; i++) {
                        for (int j = 0; j < jbound; j++) {
                            float val = exp(twoDimRead(Sij, i, j, Bc));
                            twoDimWrite(Pij, i, j, Bc, val);
                        }
                    }
                    // Compute lij(1 x Br) <- rowsum(Pij) of size(Br)
                    for (int i = 0; i < ibound; i++) {
                        float val = 0;
                        for (int j = 0; j < jbound; j++) {
                            val += twoDimRead(Pij, i, j, Bc);
                        }
                        lij[i] = val;
                    }
                    // Compute lnew(1 x Br) <- li(1 x Br) + lij(1 x Br)
                    for (int i = 0; i < ibound; i++) {
                        lnew[i] = li[i] + lij[i];
                    }
                    // Compute Oi(Br x d) <- (li(1 x Br)Oi(Br x d) + Pij(Br x Bc)Vj(Bc x d)) / lnew(1 x Br)
                    for (int i = 0; i < ibound; i++) { // Pij的行
                        for (int j = 0; j < d; j++) { // Vj的列
                            float val = li[i] * twoDimRead(Oi, i, j, d);
                            for (int k = 0; k < jbound; k++) { // Pij的列, Vj的行
                                float pij = twoDimRead(Pij, i, k, Bc);
                                float vj = twoDimRead(Vj, k, j, d);
                                val += pij * vj;
                            }
                            val = val / lnew[i];
                            twoDimWrite(Oi, i, j, d, val);
                        }
                    }
                    // Write block Oi and lnew back to O and l in main memory
                    for (int i = 0; i < ibound; i++) {
                        int in = iblock + i;
                        for (int j = 0; j < d; j++) {
                            float oi = twoDimRead(Oi, i, j, d); 
                            fourDimWrite(O, b, h, in, j, H, N, d, oi);
                        }
                        l[in] = lnew[i];
                    }
                }
            }
        }
    }

    // DO NOT EDIT THIS RETURN STATEMENT //
    // It formats your C++ Vector O back into a Tensor of Shape (B, H, N, d) and returns it //
    return torch::from_blob(O.data(), {B, H, N, d}, torch::TensorOptions().dtype(torch::kFloat32)).clone();
}


/* DO NOT EDIT THESE BINDINGS */
PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
  m.def("myNaiveAttention", &myNaiveAttention, "Naive Attention");
  m.def("myUnfusedAttentionBlocked", &myUnfusedAttentionBlocked, " Blocked Unfused Attention");
  m.def("myFusedAttention", &myFusedAttention, "Fused Attention");
  m.def("myFlashAttention", &myFlashAttention, "Flash Attention");
  m.def("twoDimRead", &twoDimRead, "twoDimRead");
  m.def("fourDimRead", &fourDimRead, "fourDimRead");
}
