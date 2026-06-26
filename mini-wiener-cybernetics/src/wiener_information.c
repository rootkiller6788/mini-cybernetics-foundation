#include "wiener_core.h"
#include "wiener_filter.h"
#include "wiener_information.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ==============================================================
 * Information Theory in Cybernetics — L2, L3, L4, L7
 *
 * Wiener (1948, Ch.III): "The notion of the amount of
 * information attaches itself very naturally to a classical
 * notion in statistical mechanics: that of entropy."
 *
 * L4: Information = -sum(p_i * log(p_i))
 * L4: Maximum entropy = log(N)
 * L4: Channel capacity C = max_{p(x)} I(X;Y)
 *
 * References:
 *   Wiener (1948) Cybernetics Ch. III
 *   Shannon (1948) Bell System Tech. J. 27:379-423,623-656
 *   Kullback & Leibler (1951) Ann. Math. Stat. 22:79-86
 * ============================================================== */

/* ==============================================================
 * DISCRETE DISTRIBUTION (L3)
 * ============================================================== */

WCDistribution* wc_dist_create(int n) {
    if (n <= 0) return NULL;
    WCDistribution* d = (WCDistribution*)calloc(1, sizeof(WCDistribution));
    if (!d) return NULL;
    d->n = n;
    d->p = (double*)calloc((size_t)n, sizeof(double));
    if (!d->p) { free(d); return NULL; }
    return d;
}

void wc_dist_free(WCDistribution* d) {
    if (!d) return;
    free(d->p);
    free(d);
}

void wc_dist_set_uniform(WCDistribution* d) {
    if (!d || d->n <= 0) return;
    double val = 1.0 / (double)d->n;
    for (int i = 0; i < d->n; i++) d->p[i] = val;
    d->is_normalized = true;
    d->entropy = log2((double)d->n);
}

double wc_dist_entropy(const WCDistribution* d) {
    if (!d || !d->p) return 0.0;
    double H = 0.0;
    for (int i = 0; i < d->n; i++) {
        double p = d->p[i];
        if (p > WC_EPSILON) H -= p * log2(p);
    }
    return H;
}

double wc_dist_kld(const WCDistribution* p, const WCDistribution* q) {
    if (!p || !q || p->n != q->n) return 0.0;
    double dkl = 0.0;
    for (int i = 0; i < p->n; i++) {
        if (p->p[i] > WC_EPSILON && q->p[i] > WC_EPSILON)
            dkl += p->p[i] * log2(p->p[i] / q->p[i]);
    }
    return dkl;
}

double wc_dist_jsd(const WCDistribution* p, const WCDistribution* q) {
    /* Jensen-Shannon divergence: JSD(P||Q) = (KL(P||M) + KL(Q||M)) / 2
     * where M = (P+Q)/2 */
    if (!p || !q || p->n != q->n) return 0.0;
    WCDistribution* m = wc_dist_create(p->n);
    if (!m) return 0.0;
    for (int i = 0; i < p->n; i++)
        m->p[i] = (p->p[i] + q->p[i]) / 2.0;
    double jsd = (wc_dist_kld(p, m) + wc_dist_kld(q, m)) / 2.0;
    wc_dist_free(m);
    return jsd;
}

bool wc_dist_is_valid(const WCDistribution* d) {
    if (!d || !d->p) return false;
    double sum = 0.0;
    for (int i = 0; i < d->n; i++) {
        if (d->p[i] < 0.0) return false;
        sum += d->p[i];
    }
    return fabs(sum - 1.0) < 0.01;
}

/* ==============================================================
 * JOINT DISTRIBUTION (L3)
 * ============================================================== */

WCJointDistribution* wc_joint_create(int n_rows, int n_cols) {
    if (n_rows <= 0 || n_cols <= 0) return NULL;
    WCJointDistribution* jd = (WCJointDistribution*)calloc(1, sizeof(WCJointDistribution));
    if (!jd) return NULL;
    jd->n_rows = n_rows;
    jd->n_cols = n_cols;
    jd->p_joint = (double*)calloc((size_t)(n_rows * n_cols), sizeof(double));
    if (!jd->p_joint) { free(jd); return NULL; }
    return jd;
}

void wc_joint_free(WCJointDistribution* jd) {
    if (!jd) return;
    free(jd->p_joint);
    free(jd);
}

double wc_joint_mutual_info(const WCJointDistribution* jd) {
    if (!jd || !jd->p_joint) return 0.0;
    /* I(X;Y) = sum_{x,y} p(x,y) * log(p(x,y) / (p(x)*p(y))) */
    double* px = (double*)calloc((size_t)jd->n_rows, sizeof(double));
    double* py = (double*)calloc((size_t)jd->n_cols, sizeof(double));
    if (!px || !py) { free(px); free(py); return 0.0; }
    for (int i = 0; i < jd->n_rows; i++)
        for (int j = 0; j < jd->n_cols; j++) {
            double p = jd->p_joint[i * jd->n_cols + j];
            px[i] += p;
            py[j] += p;
        }
    double mi = 0.0;
    for (int i = 0; i < jd->n_rows; i++)
        for (int j = 0; j < jd->n_cols; j++) {
            double p_xy = jd->p_joint[i * jd->n_cols + j];
            if (p_xy > WC_EPSILON && px[i] > WC_EPSILON && py[j] > WC_EPSILON)
                mi += p_xy * log2(p_xy / (px[i] * py[j]));
        }
    free(px); free(py);
    return mi;
}

WCDistribution* wc_joint_marginal_row(const WCJointDistribution* jd) {
    if (!jd) return NULL;
    WCDistribution* d = wc_dist_create(jd->n_rows);
    if (!d) return NULL;
    for (int i = 0; i < jd->n_rows; i++)
        for (int j = 0; j < jd->n_cols; j++)
            d->p[i] += jd->p_joint[i * jd->n_cols + j];
    d->is_normalized = true;
    return d;
}

WCDistribution* wc_joint_marginal_col(const WCJointDistribution* jd) {
    if (!jd) return NULL;
    WCDistribution* d = wc_dist_create(jd->n_cols);
    if (!d) return NULL;
    for (int j = 0; j < jd->n_cols; j++)
        for (int i = 0; i < jd->n_rows; i++)
            d->p[j] += jd->p_joint[i * jd->n_cols + j];
    d->is_normalized = true;
    return d;
}

/* ==============================================================
 * COMMUNICATION CHANNEL (L3, L4)
 *
 * L4: Channel capacity C = max_{p(x)} I(X;Y)
 * L4: For symmetric channel: C = log(N) - H(row)
 * ============================================================== */

WCChannel* wc_channel_create(int n_input, int n_output) {
    if (n_input <= 0 || n_output <= 0) return NULL;
    WCChannel* ch = (WCChannel*)calloc(1, sizeof(WCChannel));
    if (!ch) return NULL;
    ch->n_input  = n_input;
    ch->n_output = n_output;
    ch->transition = (double**)calloc((size_t)n_input, sizeof(double*));
    if (!ch->transition) { free(ch); return NULL; }
    for (int i = 0; i < n_input; i++) {
        ch->transition[i] = (double*)calloc((size_t)n_output, sizeof(double));
        if (!ch->transition[i]) { wc_channel_free(ch); return NULL; }
    }
    return ch;
}

void wc_channel_free(WCChannel* ch) {
    if (!ch) return;
    if (ch->transition) {
        for (int i = 0; i < ch->n_input; i++) free(ch->transition[i]);
        free(ch->transition);
    }
    free(ch);
}

void wc_channel_set_noise_uniform(WCChannel* ch, double error_prob) {
    if (!ch) return;
    for (int i = 0; i < ch->n_input; i++) {
        double correct = 1.0 - error_prob;
        double wrong   = error_prob / (double)(ch->n_output - 1);
        for (int j = 0; j < ch->n_output; j++)
            ch->transition[i][j] = (i == j) ? correct : wrong;
    }
}

double wc_channel_capacity(const WCChannel* ch) {
    if (!ch || ch->n_input == 0 || ch->n_output == 0) return 0.0;
    /* For symmetric channels, uniform input achieves capacity */
    double uniform_prob = 1.0 / (double)ch->n_input;
    double* py = (double*)calloc((size_t)ch->n_output, sizeof(double));
    if (!py) return 0.0;
    for (int j = 0; j < ch->n_output; j++)
        for (int i = 0; i < ch->n_input; i++)
            py[j] += uniform_prob * ch->transition[i][j];
    double H_Y = 0.0;
    for (int j = 0; j < ch->n_output; j++)
        if (py[j] > WC_EPSILON) H_Y -= py[j] * log2(py[j]);
    double H_YX = 0.0;
    for (int i = 0; i < ch->n_input; i++)
        for (int j = 0; j < ch->n_output; j++)
            if (ch->transition[i][j] > WC_EPSILON)
                H_YX -= uniform_prob * ch->transition[i][j] * log2(ch->transition[i][j]);
    free(py);
    return H_Y - H_YX;
}

double wc_channel_conditional_entropy(const WCChannel* ch, const WCDistribution* input) {
    if (!ch || !input || input->n != ch->n_input) return 0.0;
    double H_YX = 0.0;
    for (int i = 0; i < ch->n_input; i++)
        for (int j = 0; j < ch->n_output; j++)
            if (ch->transition[i][j] > WC_EPSILON)
                H_YX -= input->p[i] * ch->transition[i][j] * log2(ch->transition[i][j]);
    return H_YX;
}

WCDistribution* wc_channel_output(const WCChannel* ch, const WCDistribution* input) {
    if (!ch || !input || input->n != ch->n_input) return NULL;
    WCDistribution* out = wc_dist_create(ch->n_output);
    if (!out) return NULL;
    for (int j = 0; j < ch->n_output; j++) {
        double sum = 0.0;
        for (int i = 0; i < ch->n_input; i++)
            sum += input->p[i] * ch->transition[i][j];
        out->p[j] = sum;
    }
    out->is_normalized = true;
    return out;
}

/* ==============================================================
 * WIENER INFORMATION INTEGRAL (L3, L8)
 *
 * Wiener: I = integral(log(S(w))) dw
 * Information content of a signal is measured by the
 * integral of the log-power-spectrum over frequency.
 * ============================================================== */

WCInfoIntegral* wc_info_integral_create(int n_freq) {
    if (n_freq < 2) return NULL;
    WCInfoIntegral* wii = (WCInfoIntegral*)calloc(1, sizeof(WCInfoIntegral));
    if (!wii) return NULL;
    wii->n_freq = n_freq;
    wii->spectrum = (double*)calloc((size_t)n_freq, sizeof(double));
    wii->freq     = (double*)calloc((size_t)n_freq, sizeof(double));
    if (!wii->spectrum || !wii->freq) { wc_info_integral_free(wii); return NULL; }
    return wii;
}

void wc_info_integral_free(WCInfoIntegral* wii) {
    if (!wii) return;
    free(wii->spectrum);
    free(wii->freq);
    free(wii);
}

int wc_info_integral_compute(WCInfoIntegral* wii, const void* ps_ptr) {
    /* ps_ptr should point to a WCPowerSpectrum */
    if (!wii || !ps_ptr) return -1;
    const WCPowerSpectrum* ps = (const WCPowerSpectrum*)ps_ptr;
    if (ps->n_bins != wii->n_freq) return -1;
    memcpy(wii->spectrum, ps->psd, (size_t)wii->n_freq * sizeof(double));
    memcpy(wii->freq, ps->freq, (size_t)wii->n_freq * sizeof(double));
    /* Wiener's information integral */
    wii->info_content = 0.0;
    for (int i = 0; i < wii->n_freq; i++) {
        if (wii->spectrum[i] > WC_EPSILON)
            wii->info_content += log2(wii->spectrum[i] + 1.0);
    }
    return 0;
}

double wc_info_integral_value(const WCInfoIntegral* wii) {
    return wii ? wii->info_content : 0.0;
}

/* ==============================================================
 * CODING THEORY (L8)
 * ============================================================== */

WCCoding* wc_coding_create(void) {
    WCCoding* cd = (WCCoding*)calloc(1, sizeof(WCCoding));
    return cd;
}

void wc_coding_free(WCCoding* cd) { free(cd); }

double wc_coding_efficiency(const WCCoding* cd) {
    if (!cd || cd->source_entropy < WC_EPSILON) return 0.0;
    return cd->source_entropy / cd->coded_entropy;
}

double wc_coding_compression_ratio(const WCDistribution* source) {
    if (!source) return 1.0;
    double H = wc_dist_entropy(source);
    double H_max = wc_info_max_entropy(source->n);
    if (H_max < WC_EPSILON) return 1.0;
    return H / H_max;
}

/*
 * References:
 * Wiener (1948) Cybernetics. Ch. III "Time Series, Information,
 *   and Communication". MIT Press.
 * Shannon (1948) A Mathematical Theory of Communication.
 *   Bell System Technical Journal 27:379-423, 623-656.
 * Kullback & Leibler (1951) On information and sufficiency.
 *   Annals of Mathematical Statistics 22:79-86.
 * Lin (1991) Divergence measures based on the Shannon entropy.
 *   IEEE Trans. Information Theory 37:145-151.
 */
