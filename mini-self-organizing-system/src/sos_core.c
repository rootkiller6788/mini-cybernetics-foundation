#include "sos_core.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265358979323846
#define SOS_HISTORY_DEFAULT 1000

static double urand(void) { return (double)rand() / (double)RAND_MAX; }

SOSystem* sos_system_create(const char* name, int n_components, int n_states) {
    SOSystem* sys = (SOSystem*)calloc(1, sizeof(SOSystem));
    if (!sys) return NULL;
    sys->name = strdup(name ? name : "unnamed");
    sys->level = SOS_SELF_MAINTAINING;
    sys->regime = SOS_REGIME_NEAR_EQUILIBRIUM;
    sys->criticality = SOS_SUBCRITICAL;
    sys->n_components = n_components;
    sys->n_states = n_states;
    sys->macro_state = (double*)calloc(n_states, sizeof(double));
    sys->micro_diversity = (double*)calloc(n_components, sizeof(double));
    for (int i = 0; i < n_components; i++) sys->micro_diversity[i] = 1.0;
    sys->entropy_total = 0.0; sys->entropy_internal = 0.0;
    sys->entropy_external = 0.0; sys->free_energy = 0.0;
    sys->energy_throughput = 0.0;
    sys->n_order_params = 0; sys->order_params = NULL;
    memset(&sys->info, 0, sizeof(sys->info));
    sys->emergence = SOS_EMERGENCE_NONE; sys->emergence_strength = 0.0;
    sys->n_nodes = 0; sys->n_edges = 0;
    sys->adjacency = NULL; sys->node_degrees = NULL;
    sys->clustering_coeff = 0.0; sys->avg_path_length = 0.0;
    sys->modularity = 0.0;
    sys->history_length = 0; sys->history_capacity = SOS_HISTORY_DEFAULT;
    sys->history = (double*)malloc(SOS_HISTORY_DEFAULT * sizeof(double));
    sys->current_time = 0.0;
    sys->control_params = NULL; sys->n_control_params = 0;
    return sys;
}

void sos_system_free(SOSystem* sys) {
    if (!sys) return;
    free(sys->name); free(sys->macro_state); free(sys->micro_diversity);
    if (sys->order_params) {
        for (int i = 0; i < sys->n_order_params; i++) free(sys->order_params[i].name);
        free(sys->order_params);
    }
    if (sys->adjacency) {
        for (int i = 0; i < sys->n_nodes; i++) free(sys->adjacency[i]);
        free(sys->adjacency);
    }
    free(sys->node_degrees); free(sys->history); free(sys->control_params);
    free(sys);
}

void sos_system_set_regime(SOSystem* sys, ThermodynamicRegime regime) {
    if (!sys) return;
    sys->regime = regime;
    if (regime == SOS_REGIME_FAR_EQUILIBRIUM) sys->level = SOS_SELF_ORGANIZING;
}

void sos_system_set_level(SOSystem* sys, OrganizationLevel level) {
    if (!sys) return;
    sys->level = level;
}

void sos_add_state(SOSystem* sys, const char* name, double initial_val) {
    if (!sys || !name) return;
    int idx = sys->n_states++;
    sys->macro_state = (double*)realloc(sys->macro_state, sys->n_states * sizeof(double));
    sys->macro_state[idx] = initial_val;
}

double sos_get_state(const SOSystem* sys, int idx) {
    if (!sys || idx < 0 || idx >= sys->n_states) return 0.0;
    return sys->macro_state[idx];
}

void sos_set_state(SOSystem* sys, int idx, double value) {
    if (!sys || idx < 0 || idx >= sys->n_states) return;
    sys->macro_state[idx] = value;
}

void sos_add_order_param(SOSystem* sys, const char* name,
                         double iv, double cv, double a, double b) {
    if (!sys || !name) return;
    int idx = sys->n_order_params++;
    sys->order_params = (OrderParameter*)realloc(
        sys->order_params, sys->n_order_params * sizeof(OrderParameter));
    sys->order_params[idx].value = iv;
    sys->order_params[idx].critical_value = cv;
    sys->order_params[idx].alpha = a;
    sys->order_params[idx].beta = b;
    sys->order_params[idx].noise_intensity = 0.01;
    sys->order_params[idx].mode_index = idx;
    sys->order_params[idx].name = strdup(name);
}

void sos_record_history(SOSystem* sys) {
    if (!sys || !sys->history) return;
    if (sys->history_length >= sys->history_capacity) {
        sys->history_capacity *= 2;
        sys->history = (double*)realloc(sys->history, sys->history_capacity * sizeof(double));
    }
    double val = (sys->n_states > 0) ? sys->macro_state[0] : 0.0;
    sys->history[sys->history_length++] = val;
}

void sos_advance_time(SOSystem* sys, double dt) {
    if (!sys) return;
    sys->current_time += dt;
}

/* ===== Entropy and Thermodynamics ===== */

double sos_shannon_entropy(const double* p, int n) {
    if (!p || n <= 0) return 0.0;
    double H = 0.0;
    for (int i = 0; i < n; i++)
        if (p[i] > 1e-15) H -= p[i] * log2(p[i]);
    return H;
}

double sos_entropy_production(const SOSystem* sys, const double* rates,
                              const double* affinities, int n_reactions) {
    if (!sys || !rates || !affinities || n_reactions <= 0) return 0.0;
    double diS_dt = 0.0;
    for (int r = 0; r < n_reactions; r++)
        if (rates[r] > 1e-15 && affinities[r] > 1e-15)
            diS_dt += affinities[r] * rates[r];
    double T = sys->energy_throughput > 0.0 ? sys->energy_throughput : 1.0;
    return diS_dt / T;
}

double sos_negentropy_balance(const SOSystem* sys, double ip, double ee) {
    (void)sys; return -ip - ee;
}

double sos_glansdorff_prigogine(const double* dJ, const double* dX, int n) {
    if (!dJ || !dX || n <= 0) return 0.0;
    double dXP = 0.0;
    for (int k = 0; k < n; k++) dXP += dJ[k] * dX[k];
    return dXP;
}

double sos_free_energy(double U, double T, double S) {
    if (T < 0.0) return U;
    return U - T * S;
}

/* ===== Emergence Detection ===== */

double sos_detect_emergence(double H_macro, double H_micro) {
    if (H_micro <= 1e-15) return 0.0;
    double ratio = H_macro / H_micro;
    return (ratio > 1.0) ? 0.0 : (1.0 - ratio);
}

double sos_lmc_complexity(const double* p, int n) {
    if (!p || n <= 1) return 0.0;
    double H = sos_shannon_entropy(p, n);
    double H_max = log2((double)n);
    double H_norm = (H_max > 1e-15) ? H / H_max : 0.0;
    double D = 0.0;
    double u = 1.0 / n;
    for (int i = 0; i < n; i++) {
        double dif = p[i] - u;
        D += dif * dif;
    }
    return H_norm * D;
}

double sos_effective_complexity(double total_info, double random_info) {
    return (total_info < random_info) ? 0.0 : (total_info - random_info);
}

double sos_mutual_information(const double* joint, const double* px,
                              const double* py, int nx, int ny) {
    if (!joint || !px || !py || nx <= 0 || ny <= 0) return 0.0;
    double MI = 0.0;
    for (int i = 0; i < nx; i++)
        for (int j = 0; j < ny; j++) {
            double pxy = joint[i * ny + j];
            if (pxy > 1e-15 && px[i] > 1e-15 && py[j] > 1e-15)
                MI += pxy * log2(pxy / (px[i] * py[j]));
        }
    return MI;
}

double sos_transfer_entropy(const double* yf, const double* yp,
                            const double* xp, int n) {
    if (!yf || !yp || !xp || n < 3) return 0.0;
    double mx = 0.0, my = 0.0;
    for (int i = 0; i < n; i++) { mx += xp[i]; my += yp[i]; }
    mx /= n; my /= n;
    double vr = 0.0;
    for (int i = 0; i < n; i++) { double e = yf[i] - my; vr += e*e; }
    vr /= n;
    double sxy = 0.0, sxx = 0.0;
    for (int i = 0; i < n; i++) {
        sxy += (xp[i]-mx)*(yf[i]-my);
        sxx += (xp[i]-mx)*(xp[i]-mx);
    }
    double b = (sxx > 1e-15) ? sxy / sxx : 0.0;
    double a = my - b * mx;
    double vf = 0.0;
    for (int i = 0; i < n; i++) { double e = yf[i] - (a + b*xp[i]); vf += e*e; }
    vf /= n;
    if (vr < 1e-15 || vf < 1e-15) return 0.0;
    double te = 0.5 * log(vr / vf);
    return (te > 0.0) ? te : 0.0;
}

/* ===== Network Topology ===== */

void sos_network_init(SOSystem* sys, int n_nodes) {
    if (!sys || n_nodes <= 0) return;
    if (sys->adjacency) {
        for (int i = 0; i < sys->n_nodes; i++) free(sys->adjacency[i]);
        free(sys->adjacency);
    }
    free(sys->node_degrees);
    sys->n_nodes = n_nodes; sys->n_edges = 0;
    sys->adjacency = (double**)malloc(n_nodes * sizeof(double*));
    for (int i = 0; i < n_nodes; i++)
        sys->adjacency[i] = (double*)calloc(n_nodes, sizeof(double));
    sys->node_degrees = (double*)calloc(n_nodes, sizeof(double));
}

void sos_network_add_edge(SOSystem* sys, int src, int dst, double w) {
    if (!sys || !sys->adjacency) return;
    if (src < 0 || src >= sys->n_nodes || dst < 0 || dst >= sys->n_nodes) return;
    if (sys->adjacency[src][dst] == 0.0 && w != 0.0) sys->n_edges++;
    if (sys->adjacency[src][dst] != 0.0 && w == 0.0) sys->n_edges--;
    sys->adjacency[src][dst] = w;
    sys->node_degrees[src] += w;
    sys->node_degrees[dst] += w;
}

double sos_clustering_coefficient(const SOSystem* sys) {
    if (!sys || sys->n_nodes < 3) return 0.0;
    int tri = 0, trp = 0;
    for (int i = 0; i < sys->n_nodes; i++)
        for (int j = 0; j < sys->n_nodes; j++) {
            if (i == j) continue;
            for (int k = j+1; k < sys->n_nodes; k++) {
                if (k == i) continue;
                int c12 = (sys->adjacency[i][j]!=0.0||sys->adjacency[j][i]!=0.0);
                int c13 = (sys->adjacency[i][k]!=0.0||sys->adjacency[k][i]!=0.0);
                if (c12 && c13) {
                    trp++;
                    if (sys->adjacency[j][k]!=0.0||sys->adjacency[k][j]!=0.0) tri++;
                }
            }
        }
    return (trp > 0) ? (double)tri / trp : 0.0;
}

double sos_average_path_length(SOSystem* sys) {
    if (!sys || sys->n_nodes < 2) return 0.0;
    int n = sys->n_nodes;
    double** d = (double**)malloc(n * sizeof(double*));
    for (int i = 0; i < n; i++) {
        d[i] = (double*)malloc(n * sizeof(double));
        for (int j = 0; j < n; j++)
            d[i][j] = (i==j)?0.0:(sys->adjacency[i][j]!=0.0?1.0:1e308);
    }
    for (int k = 0; k < n; k++)
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                if (d[i][k]+d[k][j] < d[i][j])
                    d[i][j] = d[i][k]+d[k][j];
    double tot = 0.0; int cnt = 0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            if (i!=j && d[i][j] < 1e307) { tot += d[i][j]; cnt++; }
    for (int i = 0; i < n; i++) free(d[i]);
    free(d);
    return (cnt > 0) ? tot / cnt : 0.0;
}

double sos_modularity(SOSystem* sys, const int* comm) {
    if (!sys || !comm || sys->n_nodes < 2) return 0.0;
    int n = sys->n_nodes;
    double m = 0.0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            m += (sys->adjacency[i][j]!=0.0)?1.0:0.0;
    if (m < 1.0) return 0.0;
    m /= 2.0;
    double Q = 0.0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            double A = (sys->adjacency[i][j]!=0.0||sys->adjacency[j][i]!=0.0)?1.0:0.0;
            double exp = sys->node_degrees[i]*sys->node_degrees[j]/(2.0*m);
            Q += (A - exp)*(comm[i]==comm[j]?1.0:0.0);
        }
    return Q/(2.0*m);
}

/* ===== Correlation and Fluctuation ===== */

double sos_autocorrelation(const double* series, int n, int lag) {
    if (!series || n <= lag || lag < 0) return 0.0;
    double mean = 0.0;
    for (int i = 0; i < n; i++) mean += series[i];
    mean /= n;
    double num = 0.0, den = 0.0;
    for (int i = 0; i < n; i++) {
        double dev = series[i] - mean;
        den += dev*dev;
        if (i < n-lag) num += dev*(series[i+lag]-mean);
    }
    return (fabs(den)>1e-15)?num/den:0.0;
}

void sos_power_spectrum(const double* series, int n, double* spec) {
    if (!series || !spec || n < 2) return;
    int nf = n/2+1;
    for (int k = 0; k < nf; k++) {
        double re=0.0, im=0.0;
        for (int t = 0; t < n; t++) {
            double theta = 2.0*PI*k*t/n;
            re += series[t]*cos(theta);
            im -= series[t]*sin(theta);
        }
        spec[k] = (re*re+im*im)/n;
    }
}

double sos_powerlaw_exponent(const double* vals, int n, double x_min) {
    if (!vals || n <= 0) return 0.0;
    int cnt = 0; double slr = 0.0;
    for (int i = 0; i < n; i++)
        if (vals[i] >= x_min) { slr += log(vals[i]/x_min); cnt++; }
    if (cnt == 0 || slr < 1e-15) return 0.0;
    return 1.0 + (double)cnt/slr;
}

double sos_box_counting_dimension(const double* px, const double* py,
                                  int np, double mn, double mx, int ns) {
    if (!px || !py || np < 2 || ns < 2) return 0.0;
    double* lx = (double*)malloc(ns*sizeof(double));
    double* ly = (double*)malloc(ns*sizeof(double));
    double xmin=px[0], xmax=px[0], ymin=py[0], ymax=py[0];
    for (int i=0; i<np; i++) {
        if(px[i]<xmin)xmin=px[i]; if(px[i]>xmax)xmax=px[i];
        if(py[i]<ymin)ymin=py[i]; if(py[i]>ymax)ymax=py[i];
    }
    double rx=xmax-xmin, ry=ymax-ymin;
    for (int s=0; s<ns; s++) {
        double eps = mn+(double)s/(ns-1)*(mx-mn);
        if(eps<1e-15)eps=1e-10;
        int nx=(rx>1e-15)?(int)ceil(rx/eps):1;
        int ny=(ry>1e-15)?(int)ceil(ry/eps):1;
        if(nx<1)nx=1; if(ny<1)ny=1;
        char* grid=(char*)calloc(nx*ny,sizeof(char));
        for(int i=0;i<np;i++){
            int ix=(int)((px[i]-xmin)/eps),iy=(int)((py[i]-ymin)/eps);
            if(ix>=nx)ix=nx-1; if(iy>=ny)iy=ny-1;
            if(ix>=0&&iy>=0)grid[ix*ny+iy]=1;
        }
        int occ=0;
        for(int i=0;i<nx*ny;i++) if(grid[i])occ++;
        free(grid);
        lx[s]=log(1.0/eps); ly[s]=(occ>0)?log((double)occ):0.0;
    }
    double sx=0,sy=0,sxy=0,sxx=0;
    for(int i=0;i<ns;i++){sx+=lx[i];sy+=ly[i];sxy+=lx[i]*ly[i];sxx+=lx[i]*lx[i];}
    double den=ns*sxx-sx*sx;
    double D=(fabs(den)>1e-15)?(ns*sxy-sx*sy)/den:0.0;
    free(lx);free(ly);
    return (D>0.0)?D:0.0;
}

double sos_jensen_shannon_divergence(const double* p, const double* q, int n) {
    if (!p || !q || n <= 0) return 0.0;
    double jsd = 0.0;
    for (int i = 0; i < n; i++) {
        double m = 0.5*(p[i]+q[i]);
        if (p[i]>1e-15 && m>1e-15) jsd += 0.5*p[i]*log(p[i]/m);
        if (q[i]>1e-15 && m>1e-15) jsd += 0.5*q[i]*log(q[i]/m);
    }
    return jsd/log(2.0);
}

double sos_ks_entropy(const double* lyap, int n) {
    if (!lyap || n <= 0) return 0.0;
    double ks = 0.0;
    for (int i = 0; i < n; i++)
        if (lyap[i] > 0.0) ks += lyap[i];
    return ks;
}

double sos_correlation_dimension(const double** emb, int np, int ed, double eps) {
    if (!emb || np < 2 || eps <= 1e-15) return 0.0;
    int cnt=0, tot=0;
    for(int i=0;i<np;i++)
        for(int j=i+1;j<np;j++){
            double d2=0.0;
            for(int d=0;d<ed;d++){double df=emb[i][d]-emb[j][d];d2+=df*df;}
            if(sqrt(d2)<eps)cnt++;
            tot++;
        }
    if(cnt==0||tot==0)return 0.0;
    double C=(double)cnt/tot;
    return log(C)/log(eps);
}

/* ===== Bifurcation Detection ===== */

int sos_detect_bifurcation(const double* evr, const double* evi, int n) {
    if (!evr || !evi || n <= 0) return 0;
    int cnt = 0;
    for (int i=0; i<n; i++)
        if (fabs(evr[i]) < 1e-6) cnt++;
    return cnt;
}

int sos_classify_bifurcation(double lrb, double lib, double lra, double lia) {
    int sb=(lrb>1e-8)?1:(lrb<-1e-8)?-1:0;
    int sa=(lra>1e-8)?1:(lra<-1e-8)?-1:0;
    if(sb==sa && sb!=0)return 0;
    if(sb!=sa){
        if(fabs(lib)>1e-6||fabs(lia)>1e-6)return 3;
        else return 1;
    }
    if(sb==0&&sa==0){
        if(fabs(lib)>1e-6||fabs(lia)>1e-6)return 3;
        else return 4;
    }
    return 0;
}

/* ===== Math Utilities ===== */

double sos_sigmoid(double x) { return 1.0/(1.0+exp(-x)); }
double sos_logistic_map(double r, double x) { return r*x*(1.0-x); }

void sos_eigenvalues_2x2(double a, double b, double c, double d,
                         double* l1r, double* l1i,
                         double* l2r, double* l2i) {
    double tr=a+d, det=a*d-b*c, disc=tr*tr-4.0*det;
    if(disc>=0.0){
        double sd=sqrt(disc);
        *l1r=(tr+sd)/2.0; *l1i=0.0;
        *l2r=(tr-sd)/2.0; *l2i=0.0;
    }else{
        *l1r=tr/2.0; *l1i=sqrt(-disc)/2.0;
        *l2r=tr/2.0; *l2i=-sqrt(-disc)/2.0;
    }
}

void sos_rk4_step(void (*f)(double,const double*,double*,void*),
                  double t, double* x, int n, double h, void* p) {
    if(!f||!x||n<=0)return;
    double* k1=(double*)malloc(4*n*sizeof(double));
    double* k2=k1+n, *k3=k2+n, *k4=k3+n;
    double* tmp=(double*)malloc(n*sizeof(double));
    f(t,x,k1,p); for(int i=0;i<n;i++)k1[i]*=h;
    for(int i=0;i<n;i++)tmp[i]=x[i]+0.5*k1[i]; f(t+0.5*h,tmp,k2,p);
    for(int i=0;i<n;i++)k2[i]*=h;
    for(int i=0;i<n;i++)tmp[i]=x[i]+0.5*k2[i]; f(t+0.5*h,tmp,k3,p);
    for(int i=0;i<n;i++)k3[i]*=h;
    for(int i=0;i<n;i++)tmp[i]=x[i]+k3[i]; f(t+h,tmp,k4,p);
    for(int i=0;i<n;i++)k4[i]*=h;
    for(int i=0;i<n;i++)x[i]+=(k1[i]+2.0*k2[i]+2.0*k3[i]+k4[i])/6.0;
    free(k1); free(tmp);
}

double sos_max_lyapunov(void (*f)(const double*,double*,void*),
                        double* x0, int n, double d0, double dt,
                        int steps, int trans, void* p) {
    if(!f||!x0||n<=0||steps<=0)return 0.0;
    double* x=(double*)malloc(n*sizeof(double));
    double* delta=(double*)malloc(n*sizeof(double));
    double* dx=(double*)malloc(n*sizeof(double));
    for(int i=0;i<n;i++){x[i]=x0[i];delta[i]=(i==0)?d0:0.0;}
    double sl=0.0; int cnt=0;
    for(int step=0; step<trans+steps; step++){
        double* xb=(double*)malloc(n*sizeof(double));
        for(int i=0;i<n;i++)xb[i]=x[i];
        f(xb,dx,p);
        for(int i=0;i<n;i++)x[i]=xb[i]+dx[i]*dt;
        double* xp=(double*)malloc(n*sizeof(double));
        for(int i=0;i<n;i++)xp[i]=xb[i]+delta[i];
        f(xp,dx,p);
        for(int i=0;i<n;i++)delta[i]=xb[i]+delta[i]+dx[i]*dt-x[i];
        double norm=0.0;
        for(int i=0;i<n;i++)norm+=delta[i]*delta[i];
        norm=sqrt(norm);
        if(step>=trans&&norm>1e-15){sl+=log(norm/d0);cnt++;}
        if(norm>1e-15)for(int i=0;i<n;i++)delta[i]*=d0/norm;
        free(xb);free(xp);
    }
    free(x);free(delta);free(dx);
    return (cnt>0)?sl/(cnt*dt):0.0;
}

void sos_print_system(const SOSystem* sys, FILE* stream) {
    if(!sys||!stream)return;
    fprintf(stream,"=== Self-Organizing System: %s ===\n",sys->name);
    fprintf(stream,"Org Level: %d, Regime: %d, Criticality: %d\n",
            (int)sys->level,(int)sys->regime,(int)sys->criticality);
    fprintf(stream,"Components: %d, Macro States: %d\n",
            sys->n_components,sys->n_states);
    fprintf(stream,"Entropy: tot=%.4f int=%.6f ext=%.6f\n",
            sys->entropy_total,sys->entropy_internal,sys->entropy_external);
    fprintf(stream,"Free Energy: %.4f, Throughput: %.4f\n",
            sys->free_energy,sys->energy_throughput);
    fprintf(stream,"Order Params: %d\n",sys->n_order_params);
    for(int i=0;i<sys->n_order_params;i++)
        fprintf(stream,"  [%d] %s = %.4f\n",
                i,sys->order_params[i].name,sys->order_params[i].value);
    fprintf(stream,"Emergence: %d strength=%.4f\n",
            (int)sys->emergence,sys->emergence_strength);
    fprintf(stream,"Info: LMC=%.4f Shannon=%.4f\n",
            sys->info.lmc_complexity,sys->info.shannon_entropy);
    fprintf(stream,"Network: %d nodes %d edges C=%.4f L=%.4f\n",
            sys->n_nodes,sys->n_edges,
            sys->clustering_coeff,sys->avg_path_length);
    fprintf(stream,"Time: %.4f\n",sys->current_time);
}
