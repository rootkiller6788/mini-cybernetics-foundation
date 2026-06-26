#ifndef WIENER_INFORMATION_H
#define WIENER_INFORMATION_H
#include "wiener_core.h"

/* Information Theory in Cybernetics — L2, L3, L4
 * Based on: Wiener (1948) Cybernetics Ch.III
 *           Shannon (1948) Mathematical Theory of Communication
 *
 * L4: Information = negative entropy
 * L4: Channel capacity with feedback
 */

typedef struct { double* p; int n; double entropy; bool is_normalized; } WCDistribution;
typedef struct { double* p_joint; int n_rows, n_cols; } WCJointDistribution;
typedef struct { double** transition; int n_input, n_output; double capacity, noise_entropy; } WCChannel;
typedef struct { double *spectrum, *freq; int n_freq; double info_content; } WCInfoIntegral;
typedef struct { double source_entropy, coded_entropy, redundancy; int code_length; bool is_optimal; } WCCoding;

WCDistribution* wc_dist_create(int n);
void wc_dist_free(WCDistribution* d);
void wc_dist_set_uniform(WCDistribution* d);
double wc_dist_entropy(const WCDistribution* d);
double wc_dist_kld(const WCDistribution* p, const WCDistribution* q);
double wc_dist_jsd(const WCDistribution* p, const WCDistribution* q);
bool wc_dist_is_valid(const WCDistribution* d);

WCJointDistribution* wc_joint_create(int n_rows, int n_cols);
void wc_joint_free(WCJointDistribution* jd);
double wc_joint_mutual_info(const WCJointDistribution* jd);
WCDistribution* wc_joint_marginal_row(const WCJointDistribution* jd);
WCDistribution* wc_joint_marginal_col(const WCJointDistribution* jd);

WCChannel* wc_channel_create(int n_input, int n_output);
void wc_channel_free(WCChannel* ch);
void wc_channel_set_noise_uniform(WCChannel* ch, double error_prob);
double wc_channel_capacity(const WCChannel* ch);
double wc_channel_conditional_entropy(const WCChannel* ch, const WCDistribution* input);
WCDistribution* wc_channel_output(const WCChannel* ch, const WCDistribution* input);

WCInfoIntegral* wc_info_integral_create(int n_freq);
void wc_info_integral_free(WCInfoIntegral* wii);
int wc_info_integral_compute(WCInfoIntegral* wii, const void* ps);
double wc_info_integral_value(const WCInfoIntegral* wii);

WCCoding* wc_coding_create(void);
void wc_coding_free(WCCoding* cd);
double wc_coding_efficiency(const WCCoding* cd);
double wc_coding_compression_ratio(const WCDistribution* source);

#endif
